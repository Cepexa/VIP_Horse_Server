#include <set>

#include "InfoBase.hpp"

InfoBase& InfoBase::getInstance(const std::string& conninfo) {
    if (!instance) {
        instance = new InfoBase(conninfo);
    }
    return *instance;
}

InfoBase::InfoBase(const std::string& conninfo) : conn(conninfo) {
    #ifdef DEBUG
        std::cout<<"DATABASE conninfo: "<<conninfo<< std::endl;
    #endif
}

void InfoBase::registerRecord(std::shared_ptr<RecordClass> recordClass) {
    records.push_back(recordClass);
}

void InfoBase::syncDatabase() {
    for (const auto& recordClass : records) {
        updateTableSchema(*recordClass);
    }
}

// Функция для выполнения запроса без вывода данных
pqxx::result InfoBase::execute_query(const std::string& query) {
    try {
        pqxx::work txn(conn);
        pqxx::result res = txn.exec(query);
        txn.commit();
        return res;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return pqxx::result();
    }
}

bool InfoBase::execute_vector_queries(const std::vector<std::string>& queries) {
    try {
        pqxx::work txn(conn);
        for (const auto& q : queries) txn.exec(q);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return false;
    }
}

void InfoBase::garbageCollection()
{
    std::vector<std::string> queries;
    for (const auto& recClass : records)
    {
        std::string tableName = recClass->getName();
        if(tableName[0] == 'r')
        {
            queries.push_back("DELETE FROM "+tableName+" WHERE deleted != 0;");
        }
    }
    execute_vector_queries(queries);
}   

void InfoBase::updateTableSchema(RecordClass& record) {
    try {
        pqxx::work txn(conn);
        std::string tableName = record.getName();

        std::set<Field> existingColumns = fetchExistingColumns(tableName, txn);
        std::set<Field> declaredColumns(record.getFields());

        std::vector<std::string> alterQueries = generateAlterQueries(declaredColumns, existingColumns);
        std::vector<std::string> dropQueries = generateDropQueries(declaredColumns, existingColumns);

        applySchemaChanges(tableName, txn, dropQueries, alterQueries);
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка при обновлении схемы: " << e.what() << std::endl;
    }
}

std::set<Field> InfoBase::fetchExistingColumns(const std::string& tableName, pqxx::work& txn) {
    std::string query = "SELECT c.column_name, c.data_type, c.is_nullable, numeric_precision, numeric_scale, "
                        "       c.character_maximum_length, tc.constraint_type, k.table_name AS foreign_table, "
                        "c.column_default "
                        "FROM information_schema.columns c "
                        "LEFT JOIN information_schema.key_column_usage k "
                        "       ON c.column_name = k.column_name AND c.table_name = k.table_name "
                        "LEFT JOIN information_schema.table_constraints tc "
                        "       ON k.constraint_name = tc.constraint_name "
                        "LEFT JOIN information_schema.referential_constraints rc "
                        "       ON k.constraint_name = rc.constraint_name "
                        "LEFT JOIN information_schema.key_column_usage fk "
                        "       ON rc.unique_constraint_name = fk.constraint_name "
                        "WHERE c.table_name = '" + tableName + "';";
    pqxx::result res = txn.exec(query);

    std::set<Field> existingColumns;
    for (const auto& row : res) {
        std::string colName = row["column_name"].as<std::string>();
        std::string colType = row["data_type"].as<std::string>();
        std::string defaultValue = row["column_default"].is_null() ? "" : row["column_default"].c_str();
        size_t pos = defaultValue.find("::");
        if (pos != std::string::npos) {
            defaultValue = defaultValue.substr(0, pos);
        }
        Nullable isNullable = (std::string(row["is_nullable"].c_str()) == "YES") ? Nullable::NULLABLE : Nullable::NOT_NULL(defaultValue);
        FieldType type = determineFieldType(colType, row);
        bool isPrimaryKey = row["constraint_type"].c_str() == "PRIMARY KEY";
        existingColumns.emplace(colName, type, isNullable, isPrimaryKey);
    }
    return existingColumns;
}

FieldType InfoBase::determineFieldType(const std::string& colType, const pqxx::row& row) {
    if (colType == "integer") {
        std::string colConstraint = row["constraint_type"].c_str();
        std::string foreignTable = row["foreign_table"].is_null() ? "" : row["foreign_table"].c_str();
        return (colConstraint == "FOREIGN KEY") ? FieldType::REFERENCE(foreignTable) : FieldType::INTEGER;
    } else if (colType == "numeric") {
        int precision = row["numeric_precision"].is_null() ? 0 : row["numeric_precision"].as<int>();
        int scale = row["numeric_scale"].is_null() ? 0 : row["numeric_scale"].as<int>();
        return FieldType::NUMERIC_P(precision, scale);
    } else if (colType == "character varying") {
        int length = row["character_maximum_length"].is_null() ? 0 : row["character_maximum_length"].as<int>();
        return (length != 255) ? FieldType::STRING_P(length) : FieldType::STRING;
    } else if (colType == "text") {
        return FieldType::TEXT;
    } else if (colType == "boolean") {
        return FieldType::BOOLEAN;
    } else if (colType == "timestamp with time zone") {
        return FieldType::DATETIME;
    }
    return FieldType::STRING;
}

std::vector<std::string> InfoBase::generateAlterQueries(const std::set<Field>& declaredColumns, const std::set<Field>& existingColumns) {
    std::vector<std::string> alterQueries;
    for (const auto& field : declaredColumns) {
        auto it = existingColumns.find(field);
        if (it == existingColumns.end()) {
            alterQueries.push_back("ADD COLUMN " + field.getSQLDefinition(true));
        } else if (*it != field && !field.isPrimaryKey()) {
            alterQueries.push_back("DROP COLUMN " + field.getName());
            alterQueries.push_back("ADD COLUMN " + field.getSQLDefinition());
        }
    }
    return alterQueries;
}

std::vector<std::string> InfoBase::generateDropQueries(const std::set<Field>& declaredColumns, const std::set<Field>& existingColumns) {
    std::vector<std::string> dropQueries;
    for (const auto& existing : existingColumns) {
        if (declaredColumns.find(existing) == declaredColumns.end()) {
            dropQueries.push_back("DROP COLUMN " + existing.getName());
        }
    }
    return dropQueries;
}

void InfoBase::applySchemaChanges(  const std::string& tableName, 
                                    pqxx::work& txn, 
                                    const std::vector<std::string>& dropQueries, 
                                    const std::vector<std::string>& alterQueries) {
    for (const auto& dropQuery : dropQueries) {
        txn.exec("ALTER TABLE " + tableName + " " + dropQuery + ";");
    }
    if (!alterQueries.empty()) {
        std::string fullAlterSQL = "ALTER TABLE " + tableName + " " + join(alterQueries, ", ") + ";";
        txn.exec(fullAlterSQL);
    }
}

