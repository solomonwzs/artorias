as_config = {
    ["tcp_port"] = 5555,
    ["n_workers"] = 2,
    ["conn_timeout"] = 5,

    -- ["worker_type"] = 0,

    ["worker_type"] = 1,
    ["cpath"] = "./bin/?.so",
    ["worker"] = "./luas/t_socket.lua",
}
