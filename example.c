#include "oracle.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    void *oracle = CreateOracle("192.168.100.10:1521/SID", "admin", "admin");
    if (!oracle)
    {
        fprintf(stderr, "CreateOracle failed\n");
        return 1;
    }

    char *output_array = NULL;

    const char* sql = "SELECT FILM FROM MOVIES";

    int row_count = 0, col_count = 0;

    if (GetQuery(oracle, sql, &output_array, &row_count, &col_count) == OK)
    {
        printf("Query executed successfully, row count: %d, column count: %d\n", row_count, col_count);
        if (output_array)
        {
            for (int col = 0; col < col_count; col++)
            {
                for (int i = 0; i < row_count; i++)
                {
                    char *value = GetQueryValue(output_array, col, i);
                    if (value)
                    {
                        printf("row index: %d, column index: %d, value: %s\n", i, col, value);
                    }
                    else
                    {
                        fprintf(stderr, "Failed to get query value, row index: %d, column index: %d\n", i, col);
                    }
                }
            }
            free(output_array);
        }
    }
    else
    {
        fprintf(stderr, "Failed to execute query\n");
        DestroyOracle(oracle);
        return 1;
    }

    DestroyOracle(oracle);
    return 0;
}
