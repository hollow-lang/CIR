#ifndef STD_H
#define STD_H

#include <iostream>

#include "cir.h"

// TODO: extend
// TODO: format function
namespace cir_std
{
    void print(CIR& cir)
    {
        cir.getr(0).print();
        std::cout << std::endl;
    }

    namespace list
    {
        struct List
        {
            Word* data;
            size_t size;
            size_t capacity;
        };

        void new_list(CIR& cir)
        {
            Word dest = cir.getr(0); // size
            dest.expect(WordType::Integer);
            size_t cap = dest.as_int();

            List* list = new List{new Word[cap], 0, cap};
            dest = Word::from_ptr(list);
        }

        inline void free_list(CIR& cir)
        {
            List* list = (List*)cir.getr(0).as_ptr();
            delete list;
        }

        inline void append_item(CIR& cir)
        {
            List* list = (List*)cir.getr(0).as_ptr();
            if (list->size + 1 > list->capacity) // TODO: @enhancement maybe realloc instead of error
                throw std::runtime_error("List is full.");

            list->data[list->size++] = cir.getr(1);
        }

        inline void get_item(CIR& cir)
        {
            Word list_r = cir.getr(0);
            list_r.expect(WordType::Pointer);
            List* list = (List*)list_r.as_ptr();
            Word index_r = cir.getr(1);
            index_r.expect(WordType::Integer);
            size_t index = index_r.as_int();

            list_r = list->data[index];
        }

        inline void pop_item(CIR& cir)
        {
            Word list_r = cir.getr(0);
            list_r.expect(WordType::Pointer);
            List* list = (List*)list_r.as_ptr();

            list_r = list->data[list->size - 1];
            list->size--;
        }

        // TODO: @enhancement maybe add remove_item(list, i)

        void register_list(CIR& cir)
        {
            cir.set_extern_fn("std.list.new", new_list);
            cir.set_extern_fn("std.list.free", free_list);
            cir.set_extern_fn("std.list.append", append_item);
            cir.set_extern_fn("std.list.get", get_item);
            cir.set_extern_fn("std.list.pop", pop_item);
        }
    }

    void init_std(CIR& cir)
    {
        cir.set_extern_fn("std.print", print);
        list::register_list(cir);
    }
}


#endif //STD_H
