cmake_minimum_required(VERSION 3.6)

set(SRC
	WFHttpClient.cc
	WFRedisClient.cc
	WFMySQLClient.cc
	WFWebServer.cc
)

add_library(facility OBJECT ${SRC})

