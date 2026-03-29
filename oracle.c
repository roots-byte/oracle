#include "wrapper.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oci.h>
#include <time.h>
#include "oracle.h"

#define LOG_ERROR(msg, fmt, ...) Logger("ERROR", __func__, __LINE__, msg, fmt, ##__VA_ARGS__)
#define LOG_INFO(msg, fmt, ...)  Logger("INFO",  __func__, __LINE__, msg, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(msg, fmt, ...) Logger("DEBUG", __func__, __LINE__, msg, fmt, ##__VA_ARGS__)

#define COLUMN_END 0
#define LINE_END 2
#define TABLE_END 5

typedef struct
{
    char* username;
    char* password;
    char* address;
    int login;
    OCIEnv* envhp;
    OCIServer* srvhp;
    OCISvcCtx* svchp;
    OCIError* errhp;
    OCISession* usrhp;
    sword status;
    OCIStmt* stmthp;
    OCIDefine* defnp;

} OracleObject;

char* AllocateAndCopyString(const char* source)
{
    if (!source) return NULL;
    char* dest = (char*)malloc(strlen(source) + 1);
    if (dest) strcpy(dest, source);
    return dest;
}

static void Logger(const char* type, const char* func, int line, const char* message, const char* variable, ...)
{
	if(LOG_ORACLE == 0)
	{
		return;
	}

	if (!strncmp(type, "DEBUG", sizeof("DEBUG")) && !DEBUG_ORACLE)
	{
		return;
	}

    #ifdef _WIN32
        FILE* log_file = fopen("C:\\LOG\\oracle.log", "a+");
        if (log_file == NULL) log_file = fopen(".\\oracle.log", "a+");
    #else
        FILE* log_file = fopen("./oracle.log", "a+");
    #endif
        if (log_file == NULL) return;
	
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	char timestamp[32];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

	// write error in log file
	fprintf(log_file, "[%s] [%s] %s (line:%d): %s [", timestamp, type, func, line, message);
	va_list argv;
	va_start(argv, variable);
	vfprintf(log_file, variable, argv);
	va_end(argv);
	fprintf(log_file, "]\n");
    fclose(log_file);

	return;
}

void ClearOracle(OracleObject* oracle)
{
    if (oracle->stmthp) OCIHandleFree(oracle->stmthp, OCI_HTYPE_STMT);
    if (oracle->usrhp) OCISessionEnd(oracle->svchp, oracle->errhp, oracle->usrhp, OCI_DEFAULT);
    if (oracle->srvhp) OCIServerDetach(oracle->srvhp, oracle->errhp, OCI_DEFAULT);
    if (oracle->usrhp) OCIHandleFree(oracle->usrhp, OCI_HTYPE_SESSION);
    if (oracle->svchp) OCIHandleFree(oracle->svchp, OCI_HTYPE_SVCCTX);
    if (oracle->srvhp) OCIHandleFree(oracle->srvhp, OCI_HTYPE_SERVER);
    if (oracle->errhp) OCIHandleFree(oracle->errhp, OCI_HTYPE_ERROR);
    if (oracle->envhp) OCIHandleFree(oracle->envhp, OCI_HTYPE_ENV);
    oracle->login = 0;
}

void *CreateOracle(const char *oracle_adress, const char *oracle_username, const char *oracle_password)
{
    OracleObject* oracle = calloc(1, sizeof(OracleObject));
    if (!oracle)
    {
        LOG_ERROR("Error allocating memory for OracleObject", "");
        return NULL;
    }

    oracle->username = AllocateAndCopyString(oracle_username);
    oracle->password = AllocateAndCopyString(oracle_password);
    oracle->address = AllocateAndCopyString(oracle_adress);
    oracle->login = 0;
    oracle->envhp = NULL;
    oracle->srvhp = NULL;
    oracle->svchp = NULL;
    oracle->errhp = NULL;
    oracle->usrhp = NULL;
    oracle->stmthp = NULL;
    oracle->defnp = NULL;

    return (void*) oracle;
}

void DestroyOracle(void* oracle)
{
    if (oracle)
    {
        ClearOracle((OracleObject*)oracle);
        if (((OracleObject*)oracle)->username) free(((OracleObject*)oracle)->username);
        if (((OracleObject*)oracle)->password) free(((OracleObject*)oracle)->password);
        if (((OracleObject*)oracle)->address) free(((OracleObject*)oracle)->address);

        free(oracle);
    }
    return;
}

int LoginOracle(void* oracle_in)
{
    LOG_DEBUG("login was call", "");
  
    sb4 errcode;
    text errbuf[512] = {0};
    int status = 0;

    OracleObject* oracle = (OracleObject*)oracle_in;

    if (!oracle)
    {
        LOG_ERROR("invalid oracle object", "");
        return INPUT_ERROR;
    }

    for (int i = 0; i < ORACLE_ATTEMPTS; i++)
    {
        if (OCIEnvCreate(&oracle->envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL) != OCI_SUCCESS) {
            LOG_ERROR("Error OCIEnvCreate failed CreateContent", "");
            status = 2;
            ClearOracle(oracle);
            continue;
        }
        
        if(OCIHandleAlloc(oracle->envhp, (dvoid**)&oracle->errhp, OCI_HTYPE_ERROR, 0, NULL) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIHandleAlloc", "%s", errbuf);
            status = 3;
            ClearOracle(oracle);
            continue;
        }

        if (OCIHandleAlloc(oracle->envhp, (dvoid**)&oracle->srvhp, OCI_HTYPE_SERVER, 0, NULL) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIHandleAlloc", "%s", errbuf);
            status = 4;
            ClearOracle(oracle);
            continue;
        }

        if (OCIServerAttach(oracle->srvhp, oracle->errhp, (text*)oracle->address, strlen(oracle->address), OCI_DEFAULT) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIServerAttach", "%s", errbuf);
            status = 5;
            ClearOracle(oracle);
            continue;
        }

        if (OCIHandleAlloc(oracle->envhp, (dvoid**)&oracle->svchp, OCI_HTYPE_SVCCTX, 0, NULL) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIHandleAlloc", "%s", errbuf);
            status = 6;
            ClearOracle(oracle);
            continue;
        }

        if (OCIAttrSet(oracle->svchp, OCI_HTYPE_SVCCTX, oracle->srvhp, 0, OCI_ATTR_SERVER, oracle->errhp) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIAttrSet", "%s", errbuf);
            status = 7;
            ClearOracle(oracle);
            continue;
        }
        if (OCIHandleAlloc(oracle->envhp, (dvoid**)&oracle->usrhp, OCI_HTYPE_SESSION, 0, NULL) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIHandleAlloc usrhp", "%s", errbuf);
            status = 7;
            ClearOracle(oracle);
            continue;
        }
        
        if (OCIAttrSet(oracle->usrhp, OCI_HTYPE_SESSION, (void*)oracle->username, strlen(oracle->username), OCI_ATTR_USERNAME, oracle->errhp) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIAttrSet username", "%s", errbuf);
            status = 8;
            ClearOracle(oracle);
            continue;
        }

        if (OCIAttrSet(oracle->usrhp, OCI_HTYPE_SESSION, (void*)oracle->password, strlen(oracle->password), OCI_ATTR_PASSWORD, oracle->errhp) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIAttrSet password", "%s", errbuf);
            status = 9;
            ClearOracle(oracle);
            continue;
        }
        if (OCISessionBegin(oracle->svchp, oracle->errhp, oracle->usrhp, OCI_CRED_RDBMS, OCI_DEFAULT) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCISessionBegin", "%s", errbuf);
            status = 10;
            ClearOracle(oracle);
            continue;
        }

        if (OCIAttrSet(oracle->svchp, OCI_HTYPE_SVCCTX, oracle->usrhp, 0, OCI_ATTR_SESSION, oracle->errhp) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIAttrSet session", "%s", errbuf);
            status = 11;
            ClearOracle(oracle);
            continue;
        }

        if (OCIHandleAlloc(oracle->envhp, (dvoid**)&oracle->stmthp, OCI_HTYPE_STMT, 0, NULL) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("Error OCIHandleAlloc stmthp", "%s", errbuf);
            status = 12;
            ClearOracle(oracle);
            continue;
        }
        
        status = 0;
    }

    if (!status) 
    {
        LOG_DEBUG("login was succes", "");
        oracle->login = 1;
        return OK;
    }

    else
    {
        LOG_ERROR("login to database failed", "error code: %d", status);
        return LOGIN_ERROR;
    }

    return LOGIN_ERROR;
}

char** CreateValueArray(int col_count, int col_size)
{
    char** value_array = calloc(col_count, sizeof(char*));
    if (!value_array)
    {
        LOG_ERROR("Error allocating memory for value array", "");
        return NULL;
    }

    for (int i = 0; i < col_count; i++)
    {
        value_array[i] = calloc(col_size + 1, sizeof(char));
        if (!value_array[i])
        {
            LOG_ERROR("Error allocating memory for value array column", "column index: %d", i);
            for (int j = 0; j < i; j++)
            {
                free(value_array[j]);
            }
            free(value_array);
            return NULL;
        }
    }

    return value_array;
}

void DestroyValueArray(char** value_array, int col_count)
{
    if (value_array)
    {
        for (int i = 0; i < col_count; i++)
        {
            if (value_array[i])
            {
                free(value_array[i]);
            }
        }
        free(value_array);
    }
}

int GetQuery(void* oracle_in, const char* sql, char** output_array, int* row_count, int* col_count)
{
    if (DEBUG_ORACLE)
    {
        LOG_DEBUG("GetQuery was call", "sql: %s", sql);
    }

    void* tmp = NULL;
    int status = 0;
    int found_char = 0;
    int read_table;
    int max_col_size = 0;
    text errbuf[512];
    char** value_array = NULL;

    int output_array_size = 0;
    int output_array_index = 0;

    sb4 errcode;
    ub4 col_count_ub4 = 0;

    OCIDefine* defn1 = NULL, * defn2 = NULL;
    OracleObject* oracle = (OracleObject*)oracle_in;

    

    if (sql == NULL)
    {
        LOG_ERROR("sql not provided", "");
        return INPUT_ERROR;
    }

    if (!oracle_in)
    {
        LOG_ERROR("invalid oracle object", "");
        return INPUT_ERROR;
    }

    if (!output_array)
    {
        LOG_ERROR("invalid output array", "");
        return INPUT_ERROR;
    }

    *row_count = 0;
    *output_array = NULL;

    if (!oracle->login)
    {
        if (LoginOracle(oracle) != OK)
        {
            LOG_ERROR("Failed to login to Oracle database", "");
            return LOGIN_ERROR;
        }
    }

    for (int i = 0; i < ORACLE_ATTEMPTS; i++)
    {
        if (value_array != NULL)
        {
            DestroyValueArray(value_array, col_count_ub4);
            value_array = NULL;
        }

        if (i)
        {
            ClearOracle(oracle);
            status = LoginOracle(oracle);
            if (status)
            {
                LOG_ERROR("fail while try login in get query", "");
                return status;
            }
        }

        //preapere sql statement
        if (OCIStmtPrepare(oracle->stmthp, oracle->errhp, (text*)sql, strlen(sql), OCI_NTV_SYNTAX, OCI_DEFAULT) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("OCIStmtPrepare error", "error message: %s, SQL: %s", errbuf, sql);
            status = 2;
            continue;
        }

        if (OCIStmtExecute(oracle->svchp, oracle->stmthp, oracle->errhp, 0, 0, NULL, NULL, OCI_DEFAULT) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("OCIStmtExecute error", "error message: %s, SQL: %s", errbuf, sql);
            status = 66;
            continue;
        }

        if (OCIAttrGet(oracle->stmthp, OCI_HTYPE_STMT, &col_count_ub4, 0, OCI_ATTR_PARAM_COUNT, oracle->errhp) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("OCIAttrGet error", "error: %s", errbuf);
            status = 3;
            continue;
        }

        *col_count = (int)col_count_ub4;

        for (ub4 j = 1; j <= col_count_ub4; j++)
        {
            OCIParam* param = NULL;
            ub2 data_size = 0;

            OCIParamGet(oracle->stmthp, OCI_HTYPE_STMT,
                        oracle->errhp, (void**)&param, j);

            OCIAttrGet(param, OCI_DTYPE_PARAM,
                    &data_size, 0,
                    OCI_ATTR_DATA_SIZE,
                    oracle->errhp);

            if (data_size > max_col_size)
            {
                max_col_size = data_size;
            }
        }

        value_array = CreateValueArray(col_count_ub4, max_col_size);
        if (!value_array)
        {
            LOG_ERROR("Error creating value array", "column count: %d, column size: %d", col_count, max_col_size);
            return MEMORY_ERROR;
        }


        for (ub4 j = 1; j <= col_count_ub4; j++)
        {
            if (OCIDefineByPos(oracle->stmthp, &defn1, oracle->errhp, j, value_array[j-1], max_col_size, SQLT_STR, NULL, NULL, NULL, OCI_DEFAULT) != OCI_SUCCESS)
            {
                OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
                LOG_ERROR("OCIDefineByPos error", "error message: %s, SQL: %s", errbuf, sql);
                status = 3;
                break;
            }
        }

        if(status)
        {
            continue;
        }

        //execute sql statement
        if (OCIStmtExecute(oracle->svchp, oracle->stmthp, oracle->errhp, 0, 0, NULL, NULL, OCI_DEFAULT) != OCI_SUCCESS)
        {
            OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
            LOG_ERROR("OCIStmtExecute error", "error message: %s, SQL: %s", errbuf, sql);
            status = 5;
            continue;
        }

        status = 0;
        break;

    }

    if (status)
    {
        LOG_ERROR("Failed to execute query after multiple attempts", "error code: %d", status);
        if (value_array) DestroyValueArray(value_array, col_count_ub4);
        return COMUNICATION_ERROR;
    }


    *output_array = calloc(col_count_ub4 * 10, sizeof(char));
    if (!*output_array)
    {
        LOG_ERROR("Error allocating memory for output array", "size: %d", col_count_ub4 * 10);
        if (value_array) DestroyValueArray(value_array, col_count_ub4);
        return MEMORY_ERROR;
    }
    output_array_size = col_count_ub4 * 10;

    while ( (read_table = OCIStmtFetch(oracle->stmthp, oracle->errhp, 1, OCI_FETCH_NEXT, OCI_DEFAULT)) == OCI_SUCCESS || read_table == OCI_SUCCESS_WITH_INFO)
    {

        for (int col_inx = 0; col_inx < col_count_ub4; col_inx++)
        {
            value_array[col_inx][max_col_size + 1] = '\0';
            
            for (int inx = 0; inx < max_col_size + 1; inx++)
            {
                if (value_array[col_inx][inx] == '\0')
                {
                    break;
                }

                (*output_array)[output_array_index++] = value_array[col_inx][inx];

                if (output_array_index >= output_array_size)
                {
                    tmp = realloc(*output_array, output_array_size * 2 * sizeof(char));
                    if (!tmp)                    {
                        LOG_ERROR("Error reallocating memory for output array", "current size: %d", output_array_size);
                        free(*output_array);
                        *output_array = NULL;
                        if (value_array) DestroyValueArray(value_array, col_count_ub4);
                        return MEMORY_ERROR;
                    }
                    *output_array = (char*)tmp;
                    output_array_size *= 2;
                }
            }
            
            (*output_array)[output_array_index++] = COLUMN_END;
            if (output_array_index >= output_array_size)
            {
                tmp = realloc(*output_array, output_array_size * 2 * sizeof(char));
                if (!tmp)                    {
                    LOG_ERROR("Error reallocating memory for output array", "current size: %d", output_array_size);
                    free(*output_array);
                    *output_array = NULL;
                    if (value_array) DestroyValueArray(value_array, col_count_ub4);
                    return MEMORY_ERROR;
                }
                *output_array = (char*)tmp;
                output_array_size *= 2;
            }
        }
        *row_count += 1;
        (*output_array)[output_array_index++] = LINE_END;

        if (output_array_index >= output_array_size)
        {
            tmp = realloc(*output_array, output_array_size * 2 * sizeof(char));
            if (!tmp) {
                LOG_ERROR("Error reallocating memory for output array", "current size: %d", output_array_size);
                free(*output_array);
                *output_array = NULL;
                if (value_array) DestroyValueArray(value_array, col_count_ub4);
                return MEMORY_ERROR;
            }
            *output_array = (char*)tmp;
            output_array_size *= 2;
        }

    }

    (*output_array)[output_array_index] = TABLE_END;
   
    if (read_table != OCI_NO_DATA)
    {
        OCIErrorGet(oracle->errhp, 1, NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
        LOG_ERROR("OCIStmtFetch error", "return code: %d, error message: %s, SQL: %s", read_table, sql, errbuf);
        if (value_array) DestroyValueArray(value_array, col_count_ub4);
        free(*output_array);
        *output_array = NULL;
        return COMUNICATION_ERROR;

    }

    return OK;
}

char* GetQueryValue(char* output_array, int col_inx, int line_inx)
{
    int inx = -1;
    int current_col = 0;
    int current_line = 0;

    if (!output_array)
    {
        LOG_ERROR("invalid output array", "");
        return NULL;
    }

    if (col_inx < 0 || line_inx < 0)
    {
        LOG_ERROR("invalid column or line index", "column index: %d, line index: %d", col_inx, line_inx);
        return NULL;
    }

    while (output_array[++inx] != TABLE_END)
    {
        if (output_array[inx] == LINE_END)
        {
            current_line++;
            current_col = 0;
            continue;
        }
        if (output_array[inx] == COLUMN_END)
        {
            current_col++;
            continue;
        }
        if (current_col == col_inx && current_line == line_inx)
        {
            return &output_array[inx];
        }
    }

    return NULL;
}
