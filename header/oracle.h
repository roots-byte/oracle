#ifndef ORACLE_H
    #define ORACLE_H

    #ifndef LOG_ORACLE
        #define LOG_ORACLE 1
    #endif

    #ifndef DEBUG_ORACLE
        #define DEBUG_ORACLE 1
    #endif

    #ifndef ORACLE_ATTEMPTS
        #define ORACLE_ATTEMPTS 1
    #endif

    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef RESTAPI_BUILD_DLL
            #define RESTAPI_EXPORT __declspec(dllexport)
        #else
            #define RESTAPI_EXPORT
        #endif
    #else
        #define RESTAPI_EXPORT
    #endif

    enum OracleError
    {
        OK,
        LOGIN_ERROR,
        BINNING_ERROR,
        INPUT_ERROR,
        QUARY_NOT_EXIST,
        MEMORY_LEAK,
        MEMORY_ERROR,
        COMUNICATION_ERROR,
        PARSE_ERROR
    };

    /**
     * @brief Creates an Oracle handle and stores connection credentials.
     * @param[in] address Database address in host:port/service_name format.
     * @param[in] username Oracle user name.
     * @param[in] password Oracle password.
     * @return Pointer to an internal Oracle handle on success, otherwise NULL.
     * @note The returned handle is owned by the caller.
     * @note Release it by calling DestroyOracle().
     */
    RESTAPI_EXPORT void* CreateOracle(const char* address, const char* username, const char* password);

    /**
     * @brief Logs out, clears internal resources, and frees the Oracle handle.
     * @param[in] oracle Handle returned by CreateOracle().
     * @return Always returns NULL (useful pattern: oracle = DestroyOracle(oracle);).
     * @note Safe to call with NULL.
     * @warning After this call, the original handle is invalid and must not be reused.
     */
    RESTAPI_EXPORT void DestroyOracle(void* oracle);

    /**
     * @brief Executes an SQL query and returns serialized result data.
     * @param[in] oracle Oracle handle from CreateOracle().
     * @param[in] sql SQL statement to execute.
     * @param[out] output_array Allocated result buffer; caller must free(*output_array) with free().
     * @param[out] row_count Number of rows in the result set.
     * @param[out] col_count Number of columns in the result set.
     * @return Value from OracleError enum (OK on success).
     * @note On failure, *output_array is set to NULL and no free is required.
     */
    RESTAPI_EXPORT int GetQuery(void* oracle, const char* sql, char** output_array, int* row_count, int* col_count);

    /**
     * @brief Returns a cell value from the buffer produced by GetQuery().
     * @param[in] output_array Buffer returned by GetQuery().
     * @param[in] col_inx Zero-based column index.
     * @param[in] line_inx Zero-based row index.
     * @return Pointer to value text inside output_array, or NULL on error.
     * @note Do not free the returned pointer directly.
     * @warning The returned pointer becomes invalid after free(output_array).
     */
    RESTAPI_EXPORT char* GetQueryValue(char* output_array, int col_inx, int line_inx);

#endif // ORACLE_H