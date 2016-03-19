#ifndef DB_TABLE__EXHIBITS__H
#define DB_TABLE__EXHIBITS__H

#include <cstdint>
#include <string>

#include "Column.h"
#include "Field.h"
#include "SqlCore.h"

namespace db {
namespace table {

struct Exhibits {
    struct FieldID : detail::Field<std::int32_t, Exhibits> {
        using detail::Field<std::int32_t, Exhibits>::Field;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldID> ID{};

    struct FieldName : detail::Field<std::string, Exhibits> {
        using detail::Field<std::string, Exhibits>::Field;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldName> Name{};

    struct FieldVersion : detail::Field<std::int32_t, Exhibits> {
        using detail::Field<std::int32_t, Exhibits>::Field;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldVersion> Version{};

    struct FieldRgbHex : detail::Field<std::int32_t, Exhibits> {
        using detail::Field<std::int32_t, Exhibits>::Field;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldRgbHex> RgbHex{};

    struct FieldFrameX : detail::OptField<std::int32_t, Exhibits> {
        using detail::OptField<std::int32_t, Exhibits>::OptField;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldFrameX> FrameX{};

    struct FieldFrameY : detail::OptField<std::int32_t, Exhibits> {
        using detail::OptField<std::int32_t, Exhibits>::OptField;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldFrameY> FrameY{};

    struct FieldFrameWidth : detail::OptField<std::int32_t, Exhibits> {
        using detail::OptField<std::int32_t, Exhibits>::OptField;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldFrameWidth> FrameWidth{};

    struct FieldFrameHeight : detail::OptField<std::int32_t, Exhibits> {
        using detail::OptField<std::int32_t, Exhibits>::OptField;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldFrameHeight> FrameHeight{};

    struct FieldFrameFloor : detail::OptField<std::int32_t, Exhibits> {
        using detail::OptField<std::int32_t, Exhibits>::OptField;
        static const std::string columnName;
    };
    static constexpr detail::Column<FieldFrameFloor> FrameFloor{};

    static const std::string tableName;

    using Sql = detail::SqlCoreWithID<FieldID, FieldName, FieldVersion, FieldRgbHex, FieldFrameX,
                                      FieldFrameY, FieldFrameWidth, FieldFrameHeight, FieldFrameFloor>;
};
}
}

#endif