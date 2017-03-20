as_config = {
    ["tcp_port"] = 5555,
    ["n_workers"] = 2,
    ["conn_timeout"] = 5,

    -- ["worker_type"] = 0,

    ["worker_type"] = 1,
    ["lua_cpath"] = "./bin/?.so",
    ["lua_path"] = "./lua/?.lua",
    ["worker"] = "./test/t_socket.lua",
}
