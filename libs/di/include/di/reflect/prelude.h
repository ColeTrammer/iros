#pragma once

#include <di/reflect/enum_to_string.h>
#include <di/reflect/enumerator.h>
#include <di/reflect/field.h>
#include <di/reflect/format_impl.h>
#include <di/reflect/hash_impl.h>
#include <di/reflect/parse_impl.h>
#include <di/reflect/reflect.h>

namespace di {
using concepts::Reflectable;
using concepts::ReflectableToAtom;
using concepts::ReflectableToFields;
using concepts::ReflectionValue;
using meta::Reflect;

using reflection::Atom;
using reflection::atom;
using reflection::enum_to_string;
using reflection::Enumerator;
using reflection::enumerator;
using reflection::Enumerators;
using reflection::Field;
using reflection::field;
using reflection::Fields;
using reflection::make_enumerators;
using reflection::make_fields;
using reflection::reflect;
}
