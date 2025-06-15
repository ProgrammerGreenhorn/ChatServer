#定义一个SRC_LISt变量包含该目录下所有的源文件
aux_source_directory(. SRC_LIST)

aux_source_directory(./db DB_LIST)

aux_source_directory(./model MODEL_LIST)

add_executable(chatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST})

target_link_libraries(chatServer muduo_net muduo_base mysqlclient pthread) 