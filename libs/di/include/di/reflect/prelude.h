#pragma once

#include <di/reflect/field.h>
#include <di/reflect/format_impl.h>
#include <di/reflect/hash_impl.h>
#include <di/reflect/reflect.h>

namespace di {
using concepts::Reflectable;
using concepts::ReflectableToAtom;
using concepts::ReflectableToFields;
using concepts::ReflectionValue;
using meta::Reflect;

using reflection::Atom;
using reflection::atom;
using reflection::Field;
using reflection::field;
using reflection::Fields;
using reflection::make_fields;
using reflection::reflect;
}
