#pragma once

#include <pqxx/pqxx>
#include <map>
#include <memory>
#include <iostream>
#include <vector>

#include "RecordClass.hpp"
#include "FieldValue.hpp"

inline Variant convertFieldVariant(const Field& field, const pqxx::field& dbField) {
    bool isNull = dbField.is_null();
    switch (field.getType().getKind()) {
        case FieldType::Kind::REFERENCE:
        case FieldType::Kind::INTEGER:
            return isNull ? 0 : dbField.as<int>();
        case FieldType::Kind::NUMERIC:
            return isNull ? 0 : dbField.as<double>();
        case FieldType::Kind::BOOLEAN:
            return isNull ? false : dbField.as<bool>();
        case FieldType::Kind::DATETIME:
            return isNull ? DateTime{}:DateTime(dbField.as<DateTime>());
        case FieldType::Kind::STRING:
        case FieldType::Kind::TEXT:
            return isNull ? std::string{} : dbField.as<std::string>();
        default:
            throw std::runtime_error("Unsupported field type");
    }
}

class InfoBase {
public:
    // Метод getInstance() с параметром создает/возвращает экземпляр
    static InfoBase& getInstance(const std::string& conninfo = "dbname=postgres user=postgres password=admin host=127.0.0.1 port=5432");

    pqxx::result execute_query(const std::string& query);
    bool execute_vector_queries(const std::vector<std::string>& queries);
    void registerRecord(std::shared_ptr<RecordClass> record);
    void syncDatabase();
    void garbageCollection();

private:
    explicit InfoBase(const std::string& conninfo);
    InfoBase(const InfoBase&) = delete;
    InfoBase& operator=(const InfoBase&) = delete;

    static inline InfoBase* instance = nullptr;  // Статический указатель на экземпляр
    pqxx::connection conn;
    std::vector<std::shared_ptr<RecordClass>> records; // Храним указатели на RecordClass
    
    void updateTableSchema(RecordClass& record);
    std::set<Field> fetchExistingColumns(const std::string& tableName, pqxx::work& txn);
    FieldType determineFieldType(const std::string& colType, const pqxx::row& row);
    std::vector<std::string> generateAlterQueries(const std::set<Field>& declaredColumns, const std::set<Field>& existingColumns);
    std::vector<std::string> generateDropQueries(const std::set<Field>& declaredColumns, const std::set<Field>& existingColumns);
    void applySchemaChanges(const std::string& tableName, 
                            pqxx::work& txn, 
                            const std::vector<std::string>& dropQueries, 
                            const std::vector<std::string>& alterQueries);
    friend class Transaction;
};

#include <pqxx/field>
#include <pqxx/strconv>

namespace pqxx {
/** @brief Специализация nullness для DateTime. */
template<>
struct nullness<DateTime> {
    static constexpr bool has_null = true;
    static constexpr bool always_null = false;

    /** @brief Возвращает "нулевое" значение для DateTime. */
    static DateTime null() {
        return DateTime{};
    }

    /** @brief Проверяет, является ли значение "нулевым". */
    static bool is_null(const DateTime& value) {
        return value == DateTime{};
    }
};

/** @brief Специализация string_traits для DateTime. */
template<>
struct string_traits<DateTime> {
    /** @brief Преобразует строку в объект DateTime. */
    static DateTime from_string(std::string_view text) {
        return DateTime{std::string(text)};
    }
};
} // namespace pqxx