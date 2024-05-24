#include "mycc.h"

size_t size_of(Type *type) {
    switch (type->type) {
        case TY_INT:
        case TY_PTR:
            return 8;
        case TY_ARRAY:
            return size_of(type->ptr_to) * type->len;
        case TY_CHAR:
            return 1;
        default:
            error("不正なtypeです");
    }
}
