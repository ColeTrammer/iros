#pragma once

#include <liim/container/concepts.h>
#include <liim/container/view/move_elements.h>
#include <liim/result.h>

namespace LIIM::Container::Consumer {
template<typename C, Container OtherContainer>
requires(InsertableFor<C, ContainerValueType<LIIM::Container::View::MoveElements<OtherContainer>>> ||
         FalliblyInsertableFor<C, ContainerValueType<LIIM::Container::View::MoveElements<
                                      OtherContainer>>>) constexpr auto insert(C& container, ConstIteratorForContainer<C> position,
                                                                               OtherContainer&& other_container) {
    auto move_container = move_elements(forward<OtherContainer>(other_container));
    auto result = container.insert(move(position), move_container.begin(), move_container.end());
    if constexpr (Clearable<OtherContainer> && IsRValueReference<OtherContainer>::value) {
        move_container.base().clear();
    }
    return result;
}
}

using LIIM::Container::Consumer::insert;
