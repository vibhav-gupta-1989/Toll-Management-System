#pragma once

#include "SimpleJSON/json.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/json.hpp"
#include "bsoncxx/oid.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/database.hpp"
#include "mongocxx/uri.hpp"
#include "vehicleCategory.h"
#include "shiftTiming.h"

namespace TollMgmtSystem{
    constexpr char mongoDbUri[] = "mongodb://127.0.0.1:27017";
    constexpr char databaseName[] = "tollMgmtSystem";
    constexpr char routesCollectionName[] = "routes";
    constexpr char ticketsCollectionName[] = "tickets";
    constexpr char vehiclesCollectionName[] = "vehicles";
    constexpr char boothsCollectionName[] = "booths";

    class MongoDbHandler{

        public:
            MongoDbHandler()
                : uri(mongocxx::uri(mongoDbUri)),
                client(mongocxx::client(uri)),
                db(client[databaseName]){}

            bool AddStation(const std::string& sourceStationName,
                    const std::string& destinationStationName,
                    const int& toll,
                    const VehicleCategory& category){

                // Create entry in routes collection
                try {
                    mongocxx::collection collection = db[routesCollectionName];

                    auto builder = bsoncxx::builder::stream::document{};

                    std::unordered_map<VehicleCategory, std::string>::iterator it;
                    it = vehicleCategoryToString.find(category);

                    if(it == vehicleCategoryToString.end())
                        return false;

                    auto builderId = bsoncxx::builder::stream::document{};
                    bsoncxx::v_noabi::document::value doc_valueId = builderId <<
                     "sourceStationName" << sourceStationName <<
                        "destinationStationName" << destinationStationName <<
                        "vehicleCategory" << it->second << bsoncxx::builder::stream::finalize;

                    bsoncxx::v_noabi::document::value doc_value = 
                        builder << "_id" << doc_valueId 
                        << "toll" << toll << bsoncxx::builder::stream::finalize;

                    collection.insert_one(doc_value.view());
                    return true;
                } catch (const std::exception &e) {
                    return false;
                }
            }

            bool AddBooth(const int& id, const std::string& stationName,
                    const std::string& tellerName, const int& tellerId,
                    const ShiftTiming& timing){
                // Create entry in booths collection
                try{
                    mongocxx::collection collection = db[boothsCollectionName];

                    auto builder = bsoncxx::builder::stream::document{};

                    std::unordered_map<ShiftTiming, std::string>::iterator it;
                    it = shiftTimingToString.find(timing);

                    if(it == shiftTimingToString.end())
                        return false;
                    
                    auto builderId = bsoncxx::builder::stream::document{};

                    bsoncxx::v_noabi::document::value doc_valueId = builderId <<
                        "id" << id <<
                        "stationName" << stationName << bsoncxx::builder::stream::finalize;


                    bsoncxx::v_noabi::document::value doc_value = 
                        builder << "_id" << doc_valueId <<
                        "tellerName" << tellerName << "tellerId" << tellerId <<
                        "shiftTiming" << it->second << 
                        bsoncxx::builder::stream::finalize;

                    collection.insert_one(doc_value.view());
                    return true;
                }  catch (const std::exception &e) {
                    return false;
                }
            }

            bool DeleteStation(const std::string& stationName){
                // Delete entries from routes and booths collections
                try{
                    mongocxx::collection routesCollection = db[routesCollectionName];
                    mongocxx::collection boothsCollection = db[boothsCollectionName];

                    auto builder = bsoncxx::builder::stream::document{};

                    bsoncxx::document::value doc =
                        builder << "_id.sourceStationName" << stationName <<
                        bsoncxx::builder::stream::finalize;

                    routesCollection.delete_many(doc.view());

                    doc = builder << "_id.destinationStationName" << stationName <<
                        bsoncxx::builder::stream::finalize;

                    routesCollection.delete_many(doc.view());

                    doc = builder << "_id.stationName" << stationName <<
                        bsoncxx::builder::stream::finalize;

                    boothsCollection.delete_many(doc.view());

                    return true;
                }
                catch(const std::exception &e){
                    return false;
                }
            }

            bool DeleteBooth(const int& id, const std::string& stationName){
                // Delete entries from booths collection
                try{
                    mongocxx::collection boothsCollection = db[boothsCollectionName];

                    auto builder = bsoncxx::builder::stream::document{};

                    bsoncxx::document::value doc =
                        builder << "_id.id" << id << "_id.stationName" << stationName <<
                        bsoncxx::builder::stream::finalize;
                    boothsCollection.delete_one(doc.view());
                    
                    return true;

                }
                catch(const std::exception &e){
                    return false;
                }
            }

            json::JSON ShowRoutes(const std::string& sourceStationName, 
                    const std::string& destinationStationName,
                    std::vector<std::string>&vehicleCategories, std::vector<int>&tolls){

                using namespace bsoncxx::builder::basic;
                mongocxx::pipeline p{};
                p.match(make_document(kvp("_id.sourceStationName", sourceStationName),
                            kvp("_id.destinationStationName", destinationStationName)));
                p.sort(make_document(kvp("toll", 1)));
                p.limit(3);

                auto cursor = db[routesCollectionName].aggregate(p,
                        mongocxx::options::aggregate{});

                json::JSON result;
                result["tollOptions"] = json::Array();

                for(auto doc : cursor){
                    vehicleCategories.push_back(
                            doc["_id"]["vehicleCategory"].get_string().value.data());
                    tolls.push_back(
                            doc["toll"].get_int32().value);

                    result["tollOptions"].append(bsoncxx::to_json(doc));
                }

                return result;
            }

            bool PurchaseTicket(const std::string& sourceStationName,
                    const std::string& destinationStationName, const int& userId,
                    bool payingOnline){

                try{

                    std::vector<std::string>vehicleCategories;
                    std::vector<int>tolls;
                    json::JSON routes = ShowRoutes(sourceStationName, 
                            destinationStationName,
                            vehicleCategories,
                            tolls);

                    int size = routes["tollOptions"].length();
                    if(size == 0)
                        return false;

                    std::string tmpString;
                    int tmpInt;

                    while(size){
                        int chosenOption = rand() % size;

                        using namespace bsoncxx::builder::basic;
                        mongocxx::pipeline p{};
                        p.match(make_document(kvp("category", vehicleCategories[chosenOption]),
                                    kvp("userId", userId)));

                        auto cursor = db[vehiclesCollectionName].aggregate(p,
                                mongocxx::options::aggregate{});

                        if(cursor.begin() != cursor.end()){
                            // Get random registration no. Create ticket and add it 
                            // to tickets collection

                            std::vector<std::string> registrationNumbers;
                            for(auto doc : cursor){
                                registrationNumbers.push_back(
                                        doc["_id"]["registrationNumber"].get_string().value.data());
                            }

                            int chosenRegistrationNumber = rand() 
                                % registrationNumbers.size();

                            mongocxx::collection collection = db[ticketsCollectionName];

                            auto builder = bsoncxx::builder::stream::document{};

                            time_t currTime;
                            tm * currTm;
                            char dateString[50];
                            char timeString[50];

                            time(&currTime);
                            currTm = localtime(&currTime);

                            strftime(dateString, 50, "%B %d, %Y", currTm);
                            strftime(timeString, 50, "%T", currTm);

                            if(payingOnline){
                                bsoncxx::v_noabi::document::value doc_value =
                                    builder << "registrationNumber" << 
                                    registrationNumbers[chosenRegistrationNumber] <<
                                    "sourceStationName" << sourceStationName << 
                                    "destinationStationName" << destinationStationName <<
                                    "toll" << tolls[chosenOption] << "paidOnline" <<
                                    payingOnline << "startTime" << 
                                    timeString << "startDate" << dateString <<
                                    "paymentTime" << timeString << "paymentDate" <<
                                    dateString << "paymentStationName"
                                    << "" << bsoncxx::builder::stream::finalize;

                                collection.insert_one(doc_value.view());
                                return true;
                            }
                            else{
                                bsoncxx::v_noabi::document::value doc_value =
                                    builder << "registrationNumber" << 
                                    registrationNumbers[chosenRegistrationNumber] <<
                                    "sourceStationName" << sourceStationName << 
                                    "destinationStationName" << destinationStationName <<
                                    "toll" << tolls[chosenOption] << "paidOnline" <<
                                    payingOnline << "startTime" << timeString <<
                                    "startDate" << dateString << "paymentTime" <<
                                    "" << "paymentDate" << "" << "paymentStationName"
                                    << "" << bsoncxx::builder::stream::finalize;

                                collection.insert_one(doc_value.view());
                                return true;
                            }
                        }
                        else{
                            tmpString = vehicleCategories[chosenOption];
                            vehicleCategories[chosenOption] = vehicleCategories[size-1];
                            vehicleCategories[size-1] = tmpString;

                            tmpInt = tolls[chosenOption];
                            tolls[chosenOption] = tolls[size-1];
                            tolls[size-1] = tmpInt;

                            size--;

                        }
                    }
                    return false;
                }
                catch(const std::exception e){
                    return false;
                }   
            }

            json::JSON GetAmountCollected(const std::string& stationName, 
                    const std::string& date){
                using namespace bsoncxx::builder::basic;

                json::JSON result;
                
                try{
                    mongocxx::pipeline p{};

                    p.match(make_document(kvp("_id.sourceStationName", stationName)));
                    auto cursor1 = db[routesCollectionName].aggregate(p,
                                        mongocxx::options::aggregate{});
                    p = {};
                    p.match(make_document(kvp("_id.destinationStationName", stationName)));
                    auto cursor2 = db[routesCollectionName].aggregate(p,
                                        mongocxx::options::aggregate{});
                    
                    if(cursor1.begin() == cursor1.end() && cursor2.begin() == cursor2.end()){
                        result["amountCollected"] = -1;
                        return result;
                    }
                        
                    p = {};

                    p.match(make_document(kvp("paymentStationName", stationName),
                            kvp("paymentDate", date), kvp("paidOnline", false)));

                    auto cursor = db[ticketsCollectionName].aggregate(
                        p, mongocxx::options::aggregate{});

                    int amount = 0;
                    
                    if(cursor.begin() != cursor.end()){
                        for(auto doc : cursor){
                            amount += doc["toll"].get_int32().value;
                        }
                    }

                    result["amountCollected"] = amount;
                    return result;
                }
                catch(const std::exception &e){
                    result["amountCollected"] = -1;
                    return result;
                }
            }

            bool ValidateTicket(const std::string& sourceStationName,
                    const std::string& destinationStationName,
                    const std::string& currentStationName,
                    const std::string& vehicleRegistrationNumber){
                using namespace bsoncxx::builder::basic;

                mongocxx::pipeline p{};

                p.match(make_document(kvp("sourceStationName", sourceStationName),
                            kvp("destinationStationName", destinationStationName), 
                            kvp("registrationNumber", vehicleRegistrationNumber)));

                auto cursor = db[ticketsCollectionName].aggregate(
                        p, mongocxx::options::aggregate{});

                std::string startTime;
                bool paidOnline;
                std::string startDate;
                int tollOnTicket;
                std::string paymentStationName;

                std::vector<std::string>vehicleCategories;
                std::vector<int>tolls;


                if(cursor.begin() != cursor.end()){
                    for(auto doc : cursor){
                        paidOnline = doc["paidOnline"].get_bool().value;
                        startTime = doc["startTime"].get_string().value.data();
                        startDate = doc["startDate"].get_string().value.data();
                        tollOnTicket = doc["toll"].get_int32().value;
                        paymentStationName = doc["paymentStationName"].get_string().value.data();
                    }

                    if(!paidOnline && paymentStationName == ""){

                        std::cout << "Taking payment manually from the vehicle user" << std::endl;

                        // Update tickets collection with payment date, time, station
                        auto builder = bsoncxx::builder::stream::document{};
                        bsoncxx::document::value query_doc =
                            builder << "registrationNumber" << 
                            vehicleRegistrationNumber << "sourceStationName" <<
                            sourceStationName << "destinationStationName" << 
                            destinationStationName
                            << bsoncxx::builder::stream::finalize;

                        time_t currTime;
                        tm * currTm;
                        char dateString[50];
                        char timeString[50];

                        time(&currTime);
                        currTm = localtime(&currTime);

                        strftime(dateString, 50, "%B %d, %Y", currTm);
                        strftime(timeString, 50, "%T", currTm);

                        bsoncxx::document::value update_doc =
                            builder << "$set" << 
                            bsoncxx::builder::stream::open_document << 
                            "paymentTime" << timeString << "paymentDate" << dateString <<
                            "paymentStationName" << currentStationName <<
                            bsoncxx::builder::stream::close_document
                            << bsoncxx::builder::stream::finalize;

                        db[ticketsCollectionName].update_one(query_doc.view(),
                                update_doc.view());
                    }

                    struct tm storedDateTime;
                    getDateTime(storedDateTime, startDate, startTime);

                    double timeDiff = difftime(time(NULL), mktime(&storedDateTime));
                    if(timeDiff > 86400){

                        std::cout << "Ticket is older than 24 hours" << std::endl;
                        return false;
                    }
                    else{
                        if(currentStationName != sourceStationName){
                            // Get the vehicle category from vehicles collection
                            // Get fare from currentStationName to destination
                            // If the toll on the ticket > fare above, let vehicle pass. Else,
                            // ShowRoutes to the user

                            p = {};

                            p.match(make_document(kvp("_id.registrationNumber", 
                                            vehicleRegistrationNumber)));

                            cursor = db[vehiclesCollectionName].aggregate(
                                    p, mongocxx::options::aggregate{});

                            std::string category;

                            if(cursor.begin() != cursor.end()){
                                for(auto doc : cursor){
                                    category = doc["category"].get_string().value.data();
                                }
                            }

                            p = {};

                            p.match(make_document(kvp("_id.sourceStationName", currentStationName), 
                                        kvp("_id.destinationStationName", destinationStationName),
                                        kvp("_id.vehicleCategory", category)));

                            cursor = db[routesCollectionName].aggregate(
                                    p, mongocxx::options::aggregate{});

                            int toll;

                            if(cursor.begin() != cursor.end()){
                                for(auto doc : cursor){
                                    toll = doc["toll"].get_int32().value;
                                }
                            }

                            if(tollOnTicket < toll ){
                                std::cout << "Toll from current station to destination" << 
                                " station is more than toll on ticket" << std::endl;
                                return false;
                            }

                            std::cout << "Vehicle user is along the right path" << std::endl;
                            return true;

                        }
                        else
                            std::cout << "Vehicle user is along the right path" << std::endl;
                            return true;
                    }
                }
            }

            bool ValidateTest1(){
                db["routes"].delete_many({});
                db["tickets"].delete_many({});
                db["booths"].delete_many({});
                db["vehicles"].delete_many({});

                auto builder1 = bsoncxx::builder::stream::document{};
                auto builderId1 = bsoncxx::builder::stream::document{};

                auto doc_valueId1 = builderId1 << "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;

                auto doc_value = 
                        builder1 << "_id" << doc_valueId1 << 
                        "toll" << 100 << bsoncxx::builder::stream::finalize;

                db["routes"].insert_one(doc_value.view());

                auto builder2 = bsoncxx::builder::stream::document{};
                auto builderId2 = bsoncxx::builder::stream::document{};

                auto doc_valueId2 = builderId2 << "id" << 0 <<
                        "stationName" << "Badarpur" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder2 << "_id" << doc_valueId2 <<
                        "tellerName" << "Vikas" <<
                        "tellerId" << 101 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());
                
                auto builder3 = bsoncxx::builder::stream::document{};
                auto builderId3 = bsoncxx::builder::stream::document{};

                auto doc_valueId3 = builderId3 << "registrationNumber" << "HR51F5236"
                                    << bsoncxx::builder::stream::finalize;

                doc_value = 
                        builder3 << "_id" << doc_valueId3 << 
                        "category" << "TwoWheeler" <<
                        "userId" << 505 <<
                        "userName" << "Santosh" <<
                        bsoncxx::builder::stream::finalize;
                db["vehicles"].insert_one(doc_value.view());

                time_t currTime;
                tm * currTm;
                char dateString[50];
                char timeString[50];

                time(&currTime);
                currTm = localtime(&currTime);

                strftime(dateString, 50, "%B %d, %Y", currTm);
                strftime(timeString, 50, "%T", currTm);

                auto builder4 = bsoncxx::builder::stream::document{};
                doc_value = 
                        builder4 << "registrationNumber" << "HR51F5236" <<
                        "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "toll" << 100 <<
                        "paidOnline" << true <<
                        "startTime" << timeString << "startDate" << dateString <<
                        "paymentTime" << timeString << "paymentDate" << dateString <<
                        "paymentStationName" << "" <<
                        bsoncxx::builder::stream::finalize;
                db["tickets"].insert_one(doc_value.view());

                return ValidateTicket("Badarpur", "Mathura", "Badarpur", "HR51F5236");
            }

            bool ValidateTest2(){
                db["routes"].delete_many({});
                db["tickets"].delete_many({});
                db["booths"].delete_many({});
                db["vehicles"].delete_many({});

                auto builder1 = bsoncxx::builder::stream::document{};
                auto builderId1 = bsoncxx::builder::stream::document{};

                auto doc_valueId1 = builderId1 << "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;

                auto doc_value = 
                        builder1 << "_id" << doc_valueId1 << 
                        "toll" << 100 << bsoncxx::builder::stream::finalize;

                db["routes"].insert_one(doc_value.view());

                auto builder2 = bsoncxx::builder::stream::document{};
                auto builderId2 = bsoncxx::builder::stream::document{};
                auto doc_valueId2 = builderId2 << "sourceStationName" << "Faridabad" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = builder2 << "_id" << doc_valueId2 << 
                        "toll" << 50 << bsoncxx::builder::stream::finalize;
                
                db["routes"].insert_one(doc_value.view());

                auto builder3 = bsoncxx::builder::stream::document{};
                auto builderId3 = bsoncxx::builder::stream::document{};

                auto doc_valueId3 = builderId3 << "id" << 0 <<
                        "stationName" << "Badarpur" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder3 << "_id" << doc_valueId3 <<
                        "tellerName" << "Vikas" <<
                        "tellerId" << 101 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());

                auto builder4 = bsoncxx::builder::stream::document{};
                auto builderId4 = bsoncxx::builder::stream::document{};

                auto doc_valueId4 = builderId4 << "id" << 0 <<
                        "stationName" << "Faridabad" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder4 << "_id" << doc_valueId4 <<
                        "tellerName" << "Rohit" <<
                        "tellerId" << 104 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());
                
                auto builder5 = bsoncxx::builder::stream::document{};
                auto builderId5 = bsoncxx::builder::stream::document{};

                auto doc_valueId5 = builderId5 << "registrationNumber" << "HR51F5236"
                                    << bsoncxx::builder::stream::finalize;

                doc_value = 
                        builder5 << "_id" << doc_valueId5 << 
                        "category" << "TwoWheeler" <<
                        "userId" << 505 <<
                        "userName" << "Santosh" <<
                        bsoncxx::builder::stream::finalize;
                db["vehicles"].insert_one(doc_value.view());

                time_t currTime;
                tm * currTm;
                char dateString[50];
                char timeString[50];

                time(&currTime);
                currTm = localtime(&currTime);
                currTm->tm_hour -= 1;

                strftime(dateString, 50, "%B %d, %Y", currTm);
                strftime(timeString, 50, "%T", currTm);

                auto builder6 = bsoncxx::builder::stream::document{};
                doc_value = 
                        builder6 << "registrationNumber" << "HR51F5236" <<
                        "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "toll" << 100 <<
                        "paidOnline" << true <<
                        "startTime" << timeString << "startDate" << dateString <<
                        "paymentTime" << timeString << "paymentDate" << dateString <<
                        "paymentStationName" << "" <<
                        bsoncxx::builder::stream::finalize;
                db["tickets"].insert_one(doc_value.view());

                return ValidateTicket("Badarpur", "Mathura", "Faridabad", "HR51F5236");
            }

            bool ValidateTest3(){
                db["routes"].delete_many({});
                db["tickets"].delete_many({});
                db["booths"].delete_many({});
                db["vehicles"].delete_many({});

                auto builder1 = bsoncxx::builder::stream::document{};
                auto builderId1 = bsoncxx::builder::stream::document{};

                auto doc_valueId1 = builderId1 << "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;

                auto doc_value = 
                        builder1 << "_id" << doc_valueId1 << 
                        "toll" << 100 << bsoncxx::builder::stream::finalize;

                db["routes"].insert_one(doc_value.view());

                auto builder2 = bsoncxx::builder::stream::document{};
                auto builderId2 = bsoncxx::builder::stream::document{};

                auto doc_valueId2 = builderId2 << "id" << 0 <<
                        "stationName" << "Badarpur" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder2 << "_id" << doc_valueId2 <<
                        "tellerName" << "Vikas" <<
                        "tellerId" << 101 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());
                
                auto builder3 = bsoncxx::builder::stream::document{};
                auto builderId3 = bsoncxx::builder::stream::document{};

                auto doc_valueId3 = builderId3 << "registrationNumber" << "HR51F5236"
                                    << bsoncxx::builder::stream::finalize;

                doc_value = 
                        builder3 << "_id" << doc_valueId3 << 
                        "category" << "TwoWheeler" <<
                        "userId" << 505 <<
                        "userName" << "Santosh" <<
                        bsoncxx::builder::stream::finalize;
                db["vehicles"].insert_one(doc_value.view());

                time_t currTime;
                tm * currTm;
                char dateString[50];
                char timeString[50];

                time(&currTime);
                currTm = localtime(&currTime);
                currTm->tm_hour -= 2;

                strftime(dateString, 50, "%B %d, %Y", currTm);
                strftime(timeString, 50, "%T", currTm);

                auto builder4 = bsoncxx::builder::stream::document{};
                doc_value = 
                        builder4 << "registrationNumber" << "HR51F5236" <<
                        "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "toll" << 100 <<
                        "paidOnline" << false <<
                        "startTime" << timeString << "startDate" << dateString <<
                        "paymentTime" << "" << "paymentDate" << "" <<
                        "paymentStationName" << "" <<
                        bsoncxx::builder::stream::finalize;
                db["tickets"].insert_one(doc_value.view());

                return ValidateTicket("Badarpur", "Mathura", "Badarpur", "HR51F5236");
            }

            bool ValidateTest4(){
                db["routes"].delete_many({});
                db["tickets"].delete_many({});
                db["booths"].delete_many({});
                db["vehicles"].delete_many({});

                auto builder1 = bsoncxx::builder::stream::document{};
                auto builderId1 = bsoncxx::builder::stream::document{};

                auto doc_valueId1 = builderId1 << "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;

                auto doc_value = 
                        builder1 << "_id" << doc_valueId1 << 
                        "toll" << 100 << bsoncxx::builder::stream::finalize;

                db["routes"].insert_one(doc_value.view());

                auto builder2 = bsoncxx::builder::stream::document{};
                auto builderId2 = bsoncxx::builder::stream::document{};

                auto doc_valueId2 = builderId2 << "id" << 0 <<
                        "stationName" << "Badarpur" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder2 << "_id" << doc_valueId2 <<
                        "tellerName" << "Vikas" <<
                        "tellerId" << 101 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());
                
                auto builder3 = bsoncxx::builder::stream::document{};
                auto builderId3 = bsoncxx::builder::stream::document{};

                auto doc_valueId3 = builderId3 << "registrationNumber" << "HR51F5236"
                                    << bsoncxx::builder::stream::finalize;

                doc_value = 
                        builder3 << "_id" << doc_valueId3 << 
                        "category" << "TwoWheeler" <<
                        "userId" << 505 <<
                        "userName" << "Santosh" <<
                        bsoncxx::builder::stream::finalize;
                db["vehicles"].insert_one(doc_value.view());

                time_t currTime;
                tm * currTm;
                char dateString[50];
                char timeString[50];

                time(&currTime);
                currTm = localtime(&currTime);
                currTm->tm_hour -= 25;

                strftime(dateString, 50, "%B %d, %Y", currTm);
                strftime(timeString, 50, "%T", currTm);

                auto builder4 = bsoncxx::builder::stream::document{};
                doc_value = 
                        builder4 << "registrationNumber" << "HR51F5236" <<
                        "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "toll" << 100 <<
                        "paidOnline" << true <<
                        "startTime" << timeString << "startDate" << dateString <<
                        "paymentTime" << timeString << "paymentDate" << dateString <<
                        "paymentStationName" << "" <<
                        bsoncxx::builder::stream::finalize;
                db["tickets"].insert_one(doc_value.view());

                return ValidateTicket("Badarpur", "Mathura", "Badarpur", "HR51F5236");
            }

            bool ValidateTest5(){
                db["routes"].delete_many({});
                db["tickets"].delete_many({});
                db["booths"].delete_many({});
                db["vehicles"].delete_many({});

                auto builder1 = bsoncxx::builder::stream::document{};
                auto builderId1 = bsoncxx::builder::stream::document{};

                auto doc_valueId1 = builderId1 << "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;

                auto doc_value = 
                        builder1 << "_id" << doc_valueId1 << 
                        "toll" << 100 << bsoncxx::builder::stream::finalize;

                db["routes"].insert_one(doc_value.view());

                auto builder2 = bsoncxx::builder::stream::document{};
                auto builderId2 = bsoncxx::builder::stream::document{};

                auto doc_valueId2 = builderId2 << "id" << 0 <<
                        "stationName" << "Badarpur" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder2 << "_id" << doc_valueId2 <<
                        "tellerName" << "Vikas" <<
                        "tellerId" << 101 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());
                
                auto builder3 = bsoncxx::builder::stream::document{};
                auto builderId3 = bsoncxx::builder::stream::document{};

                auto doc_valueId3 = builderId3 << "registrationNumber" << "HR51F5236"
                                    << bsoncxx::builder::stream::finalize;

                doc_value = 
                        builder3 << "_id" << doc_valueId3 << 
                        "category" << "TwoWheeler" <<
                        "userId" << 505 <<
                        "userName" << "Santosh" <<
                        bsoncxx::builder::stream::finalize;
                db["vehicles"].insert_one(doc_value.view());

                time_t currTime;
                tm * currTm;
                char dateString[50];
                char timeString[50];

                time(&currTime);
                currTm = localtime(&currTime);
                currTm->tm_hour -= 25;

                strftime(dateString, 50, "%B %d, %Y", currTm);
                strftime(timeString, 50, "%T", currTm);

                auto builder4 = bsoncxx::builder::stream::document{};
                doc_value = 
                        builder4 << "registrationNumber" << "HR51F5236" <<
                        "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "toll" << 100 <<
                        "paidOnline" << false <<
                        "startTime" << timeString << "startDate" << dateString <<
                        "paymentTime" << "" << "paymentDate" << "" <<
                        "paymentStationName" << "" <<
                        bsoncxx::builder::stream::finalize;
                db["tickets"].insert_one(doc_value.view());

                return ValidateTicket("Badarpur", "Mathura", "Badarpur", "HR51F5236");
            }

            bool ValidateTest6(){
                db["routes"].delete_many({});
                db["tickets"].delete_many({});
                db["booths"].delete_many({});
                db["vehicles"].delete_many({});

                auto builder1 = bsoncxx::builder::stream::document{};
                auto builderId1 = bsoncxx::builder::stream::document{};

                auto doc_valueId1 = builderId1 << "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;

                auto doc_value = 
                        builder1 << "_id" << doc_valueId1 << 
                        "toll" << 100 << bsoncxx::builder::stream::finalize;

                db["routes"].insert_one(doc_value.view());

                auto builder2 = bsoncxx::builder::stream::document{};
                auto builderId2 = bsoncxx::builder::stream::document{};
                auto doc_valueId2 = builderId2 << "sourceStationName" << "Rohini" <<
                        "destinationStationName" << "Mathura" <<
                        "vehicleCategory" << "TwoWheeler" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = builder2 << "_id" << doc_valueId2 << 
                        "toll" << 200 << bsoncxx::builder::stream::finalize;
                
                db["routes"].insert_one(doc_value.view());

                auto builder3 = bsoncxx::builder::stream::document{};
                auto builderId3 = bsoncxx::builder::stream::document{};

                auto doc_valueId3 = builderId3 << "id" << 0 <<
                        "stationName" << "Badarpur" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder3 << "_id" << doc_valueId3 <<
                        "tellerName" << "Vikas" <<
                        "tellerId" << 101 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());
                
                auto builder4 = bsoncxx::builder::stream::document{};
                auto builderId4 = bsoncxx::builder::stream::document{};

                auto doc_valueId4 = builderId4 << "id" << 0 <<
                        "stationName" << "Rohini" << 
                        bsoncxx::builder::stream::finalize;
                
                doc_value = 
                        builder4 << "_id" << doc_valueId4 <<
                        "tellerName" << "Rohit" <<
                        "tellerId" << 105 <<
                        "shiftTiming" << "Day" << 
                        bsoncxx::builder::stream::finalize;
                db["booths"].insert_one(doc_value.view());

                auto builder5 = bsoncxx::builder::stream::document{};
                auto builderId5 = bsoncxx::builder::stream::document{};

                auto doc_valueId5 = builderId5 << "registrationNumber" << "HR51F5236"
                                    << bsoncxx::builder::stream::finalize;

                doc_value = 
                        builder5 << "_id" << doc_valueId5 << 
                        "category" << "TwoWheeler" <<
                        "userId" << 505 <<
                        "userName" << "Santosh" <<
                        bsoncxx::builder::stream::finalize;
                db["vehicles"].insert_one(doc_value.view());

                time_t currTime;
                tm * currTm;
                char dateString[50];
                char timeString[50];

                time(&currTime);
                currTm = localtime(&currTime);
                currTm->tm_hour -= 1;

                strftime(dateString, 50, "%B %d, %Y", currTm);
                strftime(timeString, 50, "%T", currTm);

                auto builder6 = bsoncxx::builder::stream::document{};
                doc_value = 
                        builder6 << "registrationNumber" << "HR51F5236" <<
                        "sourceStationName" << "Badarpur" <<
                        "destinationStationName" << "Mathura" <<
                        "toll" << 100 <<
                        "paidOnline" << true <<
                        "startTime" << timeString << "startDate" << dateString <<
                        "paymentTime" << timeString << "paymentDate" << dateString <<
                        "paymentStationName" << "" <<
                        bsoncxx::builder::stream::finalize;
                db["tickets"].insert_one(doc_value.view());

                return ValidateTicket("Badarpur", "Mathura", "Rohini", "HR51F5236");
            }

            

        private:
            mongocxx::uri uri;
            mongocxx::client client;
            mongocxx::database db;

            void getDateTime(struct tm& dateTime, std::string& startDate,
                    std::string& startTime){

                char* startDateDup = strdup(startDate.c_str());
                char* str = strtok(startDateDup, " ,");

                if(!strcmp(str, "January"))
                    dateTime.tm_mon = 0;
                else if(!strcmp(str, "February"))
                    dateTime.tm_mon = 1;
                else if(!strcmp(str, "March"))
                    dateTime.tm_mon = 2;
                else if(!strcmp(str, "April"))
                    dateTime.tm_mon = 3;
                else if(!strcmp(str, "May"))
                    dateTime.tm_mon = 4;
                else if(!strcmp(str, "June"))
                    dateTime.tm_mon = 5;
                else if(!strcmp(str, "July"))
                    dateTime.tm_mon = 6;
                else if(!strcmp(str, "August"))
                    dateTime.tm_mon = 7;
                else if(!strcmp(str, "September"))
                    dateTime.tm_mon = 8;
                else if(!strcmp(str, "October"))
                    dateTime.tm_mon = 9;
                else if(!strcmp(str, "November"))
                    dateTime.tm_mon = 10;
                else
                    dateTime.tm_mon = 11;

                str = strtok(NULL, " ,");
                dateTime.tm_mday = atoi(str);

                str = strtok(NULL, " ,");
                dateTime.tm_year = atoi(str) - 1900;

                char* startTimeDup = strdup(startTime.c_str());
                str = strtok(startTimeDup, ":");
                dateTime.tm_hour = atoi(str);

                str = strtok(NULL, ":");
                dateTime.tm_min = atoi(str);

                str = strtok(NULL, ":");
                dateTime.tm_sec = atoi(str);

                                  
            }

    };
}
