#pragma once

#include<iostream>
#include<string>

#include "SimpleJSON/json.hpp"
#include "mongocxx/instance.hpp"
#include "served/multiplexer.hpp"
#include "served/net/server.hpp"

#include "mongoDbHandler.h"
#include "vehicleCategory.h"
#include "shiftTiming.h"

namespace TollMgmtSystem {
    std::string AddStationEndPoint = "/addStation";
    constexpr char AddBoothEndPoint[] = "/addBooth";
    constexpr char DeleteStationEndPoint[] = "/deleteStation";
    constexpr char DeleteBoothEndPoint[] = "/deleteBooth";
    constexpr char ShowRoutesEndPoint[] = "/showRoutes";
    constexpr char PurchaseTicketEndPoint[] = "/purchaseTicket";
    constexpr char GetAmountCollectedEndPoint[] = "/getAmountCollected";
    constexpr char ValidateTicketEndPoint[] = "/validateTicket";
    constexpr char IpAddress[] = "127.0.0.1";
    constexpr char Port[] = "5000";
    constexpr int Threads = 10;

    class HttpServer{
        public:
            HttpServer(served::multiplexer multiplexer):
            multiplexer(multiplexer){}

            auto AddStation(){
                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());

                    auto maybeCategory = stringToVehicleCategory.find(
                        requestBody["category"].ToString());
                    
                    if(maybeCategory == stringToVehicleCategory.end())
                        return served::response::stock_reply(400, response);
                    
                    MongoDbHandler mHandler;
                    bool insertSuccessful = mHandler.AddStation(
                        requestBody["sourceStationName"].ToString(),
                        requestBody["destinationStationName"].ToString(),
                        requestBody["toll"].ToInt(),
                        maybeCategory->second);
                    
                    insertSuccessful ? served::response::stock_reply(200, response)
                                     : served::response::stock_reply(400, response);
                };
            }
            auto AddBooth(){
                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());

                    auto maybeShiftTiming = stringToShiftTiming.find(
                        requestBody["shiftTiming"].ToString());
                    
                    if(maybeShiftTiming == stringToShiftTiming.end())
                        return served::response::stock_reply(400, response);
                    
                    MongoDbHandler mHandler;
                    bool insertSuccessful = mHandler.AddBooth(
                        requestBody["id"].ToInt(),
                        requestBody["stationName"].ToString(),
                        requestBody["tellerName"].ToString(),
                        requestBody["tellerId"].ToInt(),
                        maybeShiftTiming->second);
                    
                    insertSuccessful ? served::response::stock_reply(200, response)
                                     : served::response::stock_reply(400, response);
                };
            }
            auto DeleteStation(){
                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());
                    MongoDbHandler mHandler;

                    bool deleteSuccessful = mHandler.DeleteStation(
                                                requestBody["stationName"].ToString());
                    
                    deleteSuccessful ? served::response::stock_reply(200, response)
                                     : served::response::stock_reply(404, response);
                };

            }
            auto DeleteBooth(){
                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());
                    MongoDbHandler mHandler;

                    bool deleteSuccessful = mHandler.DeleteBooth(
                                                requestBody["id"].ToInt(),
                                                requestBody["stationName"].ToString());
                    
                    deleteSuccessful ? served::response::stock_reply(200, response)
                                     : served::response::stock_reply(404, response);
                };
            }
            auto ShowRoutes(){

                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());
                    MongoDbHandler mHandler;

                    std::vector<std::string>vehicleCategories;
                    std::vector<int>tolls;
                    json::JSON all_routes = mHandler.ShowRoutes(
                                                requestBody["sourceStationName"].ToString(),
                                                requestBody["destinationStationName"].ToString(),
                                                vehicleCategories, tolls);
                    
                    std::ostringstream stream;
                    stream << all_routes;
                    response << stream.str();
                };

            }
            auto PurchaseTicket(){
                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());
                    MongoDbHandler mHandler;

                    bool purchaseSuccessful = mHandler.PurchaseTicket(
                                                requestBody["sourceStationName"].ToString(),
                                                requestBody["destinationStationName"].ToString(),
                                                requestBody["userId"].ToInt(),
                                                requestBody["payingOnline"].ToBool()    
                                                );
                    
                    purchaseSuccessful ? served::response::stock_reply(200, response)
                                     : served::response::stock_reply(404, response);
                };
            }
            auto GetAmountCollected(){
                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());
                    MongoDbHandler mHandler;

                    json::JSON amount = mHandler.GetAmountCollected(
                                                requestBody["stationName"].ToString(),
                                                requestBody["date"].ToString());

                    if(amount["amountCollected"].ToInt() == -1){
                        served::response::stock_reply(400, response);
                    }
                    else{
                        std::ostringstream stream;
                        stream << amount;
                        response << stream.str();
                    }
                };
            }
            auto ValidateTicket(){
                return [&](served::response &response, const served::request &request){
                    json::JSON requestBody = json::JSON::Load(request.body());
                    MongoDbHandler mHandler;

                    bool isValid = mHandler.ValidateTicket(
                                    requestBody["sourceStationName"].ToString(),
                                    requestBody["destinationStationName"].ToString(),
                                    requestBody["currentStationName"].ToString(),
                                    requestBody["vehicleRegistrationNumber"].ToString());
                    
                    if(!isValid){
                        
                        std::vector<std::string>vehicleCategories;
                        std::vector<int>tolls;
                        json::JSON routes = mHandler.ShowRoutes(
                                            requestBody["currentStationName"].ToString(),
                                            requestBody["destinationStationName"].ToString(),
                                            vehicleCategories, tolls);
                        
                        std::ostringstream stream;
                        stream << routes;
                        response << stream.str();
                    }
                    else{
                        served::response::stock_reply(200, response);
                    }
                };
            }
            
            void InitializeEndPoints(){
                multiplexer.handle(AddStationEndPoint).post(AddStation());
                multiplexer.handle(AddBoothEndPoint).post(AddBooth());
                multiplexer.handle(DeleteStationEndPoint).post(DeleteStation());
                multiplexer.handle(DeleteBoothEndPoint).post(DeleteBooth());
                multiplexer.handle(ShowRoutesEndPoint).post(ShowRoutes());
                multiplexer.handle(PurchaseTicketEndPoint).post(PurchaseTicket());
                multiplexer.handle(GetAmountCollectedEndPoint).post(GetAmountCollected());
                multiplexer.handle(ValidateTicketEndPoint).post(ValidateTicket());
            }

            void StartServer(){
                mongocxx::instance instance;
                served::net::server server(IpAddress, Port, multiplexer);
                std::cout << "Starting server to listen on port " << Port << "..." << std::endl;
                server.run(Threads);
            }

        private:
            served::multiplexer multiplexer;
    };
}