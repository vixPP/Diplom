cmake_minimum_required(VERSION 3.20)


add_executable(DataBaseApp 
	main.cpp
	DBSearchSystem.cpp
	)

target_compile_features(DataBaseApp PRIVATE cxx_std_17) 

target_link_libraries(DataBaseApp pqxx)  

target_include_directories(DataBaseApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(DataBaseApp ${Boost_LIBRARIES})

target_link_libraries(DataBaseApp OpenSSL::SSL)