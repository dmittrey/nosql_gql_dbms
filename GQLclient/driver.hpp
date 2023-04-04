#pragma once

#include <fstream>
#include <iostream>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <numeric>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>
#include <boost/serialization/vector.hpp>

#include <FlexLexer.h>
#include "parser.tab.hh"

#include "node.hpp"

#include "dbms_client.hpp"

namespace yy
{
    class Driver
    {
        FlexLexer *plex_;
        DataBaseClient *client_;
        std::vector<QueryNode> *queryList = new std::vector<QueryNode>{};

        std::ostringstream archive_stream{};
        boost::archive::xml_oarchive oa{archive_stream};

        const std::unordered_map<std::string, Cmp> cmpTable = {{"GT", Cmp::GT}, {"gt", Cmp::GT}, {"GE", Cmp::GE}, {"ge", Cmp::GE}, {"LT", Cmp::LT}, {"lt", Cmp::LT}, {"LE", Cmp::LE}, {"le", Cmp::LE}, {"EQ", Cmp::EQ}, {"eq", Cmp::EQ}, {"IN", Cmp::IN}, {"in", Cmp::IN}};
        const std::unordered_map<std::string, Command> commandTable = {{"insert", Command::INSERT}, {"INSERT", Command::INSERT}, {"delete", Command::DELETE}, {"DELETE", Command::DELETE}, {"update", Command::UPDATE}, {"UPDATE", Command::UPDATE}, {"select", Command::SELECT}, {"SELECT", Command::SELECT}};

    public:
        Driver(FlexLexer *plex, DataBaseClient *client) : plex_(plex), client_(client) {}

        parser::token_type yylex(parser::semantic_type *yylval)
        {
            parser::token_type tt = static_cast<parser::token_type>(plex_->yylex());

            switch (tt)
            {
            case yy::parser::token_type::WORD:
                yylval->as<std::string>() = plex_->YYText();
                break;
            case yy::parser::token_type::STRING:
                yylval->as<std::string>() = plex_->YYText();
                yylval->as<std::string>().erase(
                    remove(yylval->as<std::string>().begin(), yylval->as<std::string>().end(), '\"'),
                    yylval->as<std::string>().end());
                break;
            case yy::parser::token_type::INT:
                yylval->as<int>() = std::stoi(plex_->YYText());
                break;
            case yy::parser::token_type::DOUBLE:
                yylval->as<double>() = std::stod(plex_->YYText());
                break;
            case yy::parser::token_type::BOOL:
                yylval->as<bool>() = (strcmp(plex_->YYText(), "true") == 0 || strcmp(plex_->YYText(), "TRUE") == 0);
                break;
            case yy::parser::token_type::CMP:
            case yy::parser::token_type::EQ:
            case yy::parser::token_type::IN:
                yylval->as<Cmp>() = cmpTable.find(plex_->YYText())->second;
                break;
            case yy::parser::token_type::INSERT:
            case yy::parser::token_type::DELETE:
            case yy::parser::token_type::SELECT:
            case yy::parser::token_type::UPDATE:
                yylval->as<Command>() = commandTable.find(plex_->YYText())->second;
                break;
            default:
                break;
            }

            return tt;
        }

        bool parse()
        {
            parser parser(this);
            return parser.parse();
        }

        void insert(QueryNode &query)
        {
            std::cout << query.repr(0) << std::endl;

            Request *request = query.toRequest();

            oa << BOOST_SERIALIZATION_NVP(request);
            std::string outbound_data_ = archive_stream.str();
            std::cout << outbound_data_ << std::endl;
            client_->Apply(outbound_data_);
        }
    };
}