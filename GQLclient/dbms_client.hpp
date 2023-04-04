#pragma once

#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp> 

#include "dbms.grpc.pb.h"

using dbms::DataBase;
using dbms::HelloRequest;
using dbms::HelloResponse;
using dbms::OperationRequest;
using dbms::OperationResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

class DataBaseClient
{
private:
    std::unique_ptr<DataBase::Stub> stub_;

public:
    DataBaseClient(uint16_t port) : DataBaseClient("localhost", port) {}

    DataBaseClient(std::string address, uint16_t port)
        : stub_(DataBase::NewStub(
              grpc::CreateChannel(address + ":" + std::to_string(port),
                                  grpc::InsecureChannelCredentials()))) {}

    bool Ping()
    {
        HelloRequest request;
        request.set_message("Ping");
        HelloResponse reply;
        ClientContext context;

        Status status = stub_->Ping(&context, request, &reply);

        if (status.ok())
        {
            std::cout << "Received from server: " << reply.message() << std::endl;
            return true;
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return false;
        }
    }

    void Apply(std::string xmlMessage)
    {
        ClientContext context;

        OperationRequest request;
        request.set_xml(xmlMessage);

        OperationResponse response;

        std::unique_ptr<ClientReader<OperationResponse>> reader(
            stub_->Apply(&context, request));

        while (reader->Read(&response))
        {
            std::cout << "Hello" << std::endl;
            std::cout << response.xml() << std::endl;

            Request request;
            std::stringstream ssi{response.xml()};
            boost::archive::text_iarchive oa{ssi, boost::archive::no_header};
            std::cout << "Helo" << std::endl;
            oa >> request;

            std::cout << request.type_name << std::endl;
        }
        Status status = reader->Finish();
        if (status.ok())
        {
            std::cout << "OperationResponse rpc succeeded." << std::endl;
        }
        else
        {
            std::cout << "OperationResponse rpc failed." << std::endl;
        }
    }
};