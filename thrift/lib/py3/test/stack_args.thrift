struct simple {
    1: i32 val,
}

service StackService {
    list<i32> add_to(1: list<i32> lst, 2: i32 value)
    simple get_simple()
    void take_simple(1: simple smpl)
}
