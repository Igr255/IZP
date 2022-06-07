/** @file sheet.c
*
* @brief Chart editation 
* @author Igor Hanus (xhanus19)
*
* @date November 2020 (academic year 2020/2021)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define MAX_LEN 10242

// used for easier command recognition in switch

#define ACOL 1
#define AROW 2
#define DROW 3
#define DCOL 4
#define IROW 5
#define ICOL 6
#define DROWS 7
#define DCOLS 8
#define CSET 9
#define TOLOWER 10
#define TOUPPER 11
#define ROUND 12
#define INT 13
#define COPY 14
#define SWAP 15
#define MOVE 16
#define ROWS 17
#define BEGINSWITH 18
#define CONTAINS 19

struct argument {
    int id;
    int param1;
    int param2;
    char *strParam;
};

void
readArgs(int argc, char *argv[], struct argument argList[200], bool *isInsertColActive, int *iColCount, bool *warning,
         int *aRowCount);

char *loadLine(char *buffer, bool *warning, bool *isLastLine, int *tempSymbol, bool *isFirstLoad);

int getColCount(char *row, char delim, int rowLen);

void setArguments(char delim, char *row, int arg1, int arg2, char *strArg, int rowNum, int argId, int colCount,
                  bool applyOnRow, bool *deleteRow, bool *printLine, bool *warning, bool *isInsertColActive,
                  int *iColCount);

void setSelectionArguments(int rowNum, bool *applyOnRow, bool *rowSelection, char delim, char *row, int argId, int arg1,
                           int arg2, char *strArg, bool *isLastLine);

void moveCol(char delim, char *row, int arg1, int arg2, bool *warning, int colCount);

void DelimReplace(char indent, char toReplace, char *line);


int main(int argc, char *argv[]) {
    char buffer[MAX_LEN];
    char delim = ' ';

    struct argument argList[100];
    memset(argList, 0, 100 * sizeof(struct argument)); // sets values in argList to 0 so it doesn't have to iterate trough the whole array

    int colCount = 0;
    int firstColCount = 0;
    int rowNum = 1;
    int bufferLen;
    int argPos;
    int iColCount = 0;
    int aRowCount = 0;
    int tempSymbol = 1;

    bool deleteRow = false;
    bool printLine = false;
    bool applyOnRow = false;
    bool rowSelection = false;
    bool warning = false;
    bool isInsertColActive = false;
    bool isLastLine = false;
    bool isFirstLoad = true;
    bool firstColLoad = true;

    readArgs(argc, argv, argList, &isInsertColActive, &iColCount, &warning, &aRowCount);

    if (warning) { // if warning is triggered by an existing error, program ends
        return -1;
    }

    while (*loadLine(buffer, &warning, &isLastLine, &tempSymbol, &isFirstLoad) != '\0') {
        bufferLen = strlen(buffer);
        if (argc > 2 && strcmp(argv[1], "-d") == 0) {

            delim = argv[2][0];
            int argSize = strlen(argv[2]);

            for (int i = 1; i < argSize; i++) {
                DelimReplace(delim, argv[2][i], buffer);
            }
        }

        colCount = getColCount(buffer, delim, bufferLen);
        if (firstColLoad) {
            firstColCount = colCount;
            firstColLoad = false;
        }

        if (colCount != firstColCount) {
            fprintf(stderr, "Incorrect chart formatting\n");
            return -1;
        }

        argPos = 0;

        while (argList[argPos].id != 0) { // interating trough valid arguments
            setSelectionArguments(rowNum, &applyOnRow, &rowSelection, delim, buffer, argList[argPos].id,
                                  argList[argPos].param1, argList[argPos].param2, argList[argPos].strParam,
                                  &isLastLine);
            if (argList[argPos].id < 17) {
                setArguments(delim, buffer, argList[argPos].param1, argList[argPos].param2, argList[argPos].strParam,
                             rowNum, argList[argPos].id, colCount, applyOnRow, &deleteRow, &printLine, &warning,
                             &isInsertColActive, &iColCount);
            }
            argPos++;
        }
        if (warning) {
            return -1;
        }
        if (!deleteRow) {
            printf("%s", buffer);
        }
        rowNum++;
    }

    if (printLine) { // printing a new row at the end of the chart
        if (isInsertColActive) {
            colCount += iColCount;
        }
        for (int i = 0; i < aRowCount; i++) {
            for (int l = 0; l < colCount; l++) {
                printf("%c", delim);
            }
        }
        printf("\n");
    }
}

void printError(bool *warning, int errID, char *errAtCommand) {
    if (errID == 0) {
        fprintf(stderr, "Could not perfom the action(Outside of bounds of the array)\n");
    } else if (errID == 1) {
        fprintf(stderr, "The column number you inserted does not exist\n");
    } else if (errID == 5) {
        fprintf(stderr, "Incorrect arguments at command \"%s\". No change has been made. \n\n", errAtCommand);
    }
    *warning = true;
}

void DelimReplace(char indent, char toReplace, char *line) { // replaces all delims to a main one
    int lineLength = strlen(line);
    for (int i = 0; i < lineLength; i++) {
        if (line[i] == toReplace) {
            line[i] = indent;
        }
    }
}

void
readArgs(int argc, char *argv[], struct argument argList[100], bool *isInsertColActive, int *iColCount, bool *warning,
         int *aRowCount) {
    char *argName[19] = {"acol", "arow", "drow", "dcol", "irow", "icol", "drows", "dcols", "cset", "tolower", "toupper",
                         "round", "int", "copy", "swap", "move", "rows", "beginswith", "contains"}; // storing commands and their argument values
    int argCount[19] = {0, 0, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    struct argument args;
    int argPosition = 0;
    args.param1 = args.param2 = args.id = 0;
    args.strParam = " ";

    for (int i = 0; i < argc; i++) {
        for (int j = 0; j < 19; j++) {

            if (strcmp(argv[i], argName[j]) == 0) {
                if (strcmp(argv[i], "icol") == 0 || strcmp(argv[i], "acol") == 0) { // checks if iCol is included so iRow knows that there is one more column needed
                    *isInsertColActive = true;
                    *iColCount += 1;
                } else if (strcmp(argv[i], "arow") == 0) { // aRow counter
                    *aRowCount += 1;
                }

                args.id = j + 1; // assigning argument id so i do nto have to use strings to compare
                if (argCount[j] >= 1) { // checks if there are any arguments available
                    if (i + 1 <= argc - 1) { // checking if there are available parameters for the argument
                        if (isdigit(*argv[i + 1])) { // every argument's first parameter is either a number or '-'

                            if (atoi(argv[i + 1]) < 1) {
                                return printError(warning, 5, argv[i]);
                            }

                            args.param1 = atoi(argv[i + 1]); // assigning argument value as parameters
                        } else {
                            if (strcmp(argv[i], "rows") == 0 &&
                                strcmp(argv[i + 1], "-") == 0) { // specific condition for rows command
                                args.param1 = -1;
                            } else {
                                return printError(warning, 5, argv[i]);
                            }
                        }
                    }
                }

                if (argCount[j] == 2) {
                    if (i + 2 <= argc - 1) { // checking if there are available parameters for the argument
                        if (strcmp(argv[i], "rows") == 0 || strcmp(argv[i], "dcols") == 0 ||
                            strcmp(argv[i], "drows") == 0 || strcmp(argv[i], "dcols") == 0) { // specific condition of shown commands

                            if (isdigit(*argv[i + 2])) { // assign parameter if it is a number
                                args.param2 = atoi(argv[i + 2]);
                            } else if (strcmp(argv[i + 2], "-") == 0 && strcmp(argv[i], "rows") == 0) { // if seconds parameter of rows argument is - assign -1
                                args.param2 = -1;
                            } else {
                                return printError(warning, 5, argv[i]);
                            }
                            if (args.param2 < args.param1 && args.param2 != -1) { // check if parameter 2 < parameter 1
                                return printError(warning, 5, argv[i]);
                            }

                        } else {
                            if (isdigit(*argv[i + 2]) && strcmp(argv[i], "cset") != 0 && 
                                strcmp(argv[i], "beginswith") != 0 && strcmp(argv[i], "contains") != 0) { // special condition for named arguments
                                if (atoi(argv[i + 2]) < 1) { // checking if arg > 0
                                    return printError(warning, 5, argv[i]);
                                }
                                args.param2 = atoi(argv[i + 2]); // assigning int value
                            } else {
                                if ((strcmp(argv[i], "cset") == 0 || strcmp(argv[i], "beginswith") == 0 || 
                                     strcmp(argv[i], "contains") == 0) && strlen(argv[i + 2]) < 101) { // special condition for named arguments
                                    args.strParam = argv[i + 2]; // is it is not a nubmer assign value to sting parameter
                                } else {
                                    return printError(warning, 5, argv[i]);
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
        if (args.id != 0) { // clearing all values for the next iteration
            argList[argPosition] = args;
            args.param1 = args.param2 = args.id = 0;
            args.strParam = " ";
            argPosition++;
        }
    }
}

// with this function i wanted to try to make my own load function instead of simply using fgets
char *loadLine(char *buffer, bool *warning, bool *isLastLine, int *tempSymbol, bool *isFirstLoad) {
    int symbol;
    int i = 0;
    memset(buffer, '\0', MAX_LEN);

    if (!(*isFirstLoad)) {
        if (*tempSymbol != EOF) {
            buffer[i] = *tempSymbol;
            i++;
        }
    }
    *isFirstLoad = false;

    while ((symbol = getchar()) != EOF && *tempSymbol != EOF) {

        if (strlen(buffer) <= MAX_LEN - 2 && strlen(buffer) < 101) { // checks if the length is 100 characters (stated in code definition) or if it's not outside of 10kiB limit
            if (symbol == '\n') {

                if ((*tempSymbol = getchar()) == EOF) {
                    *isLastLine = true; // triggers the last line bool for rows - - command
                }

                buffer[i] = '\n'; // palces newline to the end of the buffer
                buffer[i + 1] = '\0';
                return buffer;
            }
            buffer[i] = symbol; // appends buffer with foudn symbol
            i++;
        } else {
            fprintf(stderr, "The line is too long\n");
            *warning = true;
            return "\0";
        }
    }
    if (strlen(buffer) <= MAX_LEN - 2 && strlen(buffer) < 101) { // final check before returning the line
        buffer[i] = '\0';
        return buffer;
    } else {
        fprintf(stderr, "The line is too long\n");
        *warning = true;
        return "\0";
    }
}

bool dRow(int arg1, int arg2, int rowNum) { // deletes specified row
    for (int i = arg1; i < arg2 + 1; i++) {
        if (i == rowNum) {
            return true;
        }
    }
    return false;
}

char *findCol(char *tempArray, char *row, char delim, int arg1) { // searching for a specific col at give index
    int delimSplit = arg1 - 1;
    int counter = 0;
    int rowLen = strlen(row);
    int k = 0;
    int l = 0;
    int tempArrayLen = strlen(tempArray);
    for (int j = 0; j < tempArrayLen; j++) { // cleaning the array
        tempArray[j] = '\0';
    }
    for (int i = 0; i < rowLen + 1; i++) {
        if (counter == delimSplit) {
            k = i;
            while (row[k] != delim && row[k] != '\n' &&
                   row[k] != '\0') { // adding the specific column to the temporary array
                tempArray[l] = row[k];
                l++;
                k++;
            }
            break;
        }
        if (row[i] == delim) {
            counter++;
        }
    }
    return tempArray;
}

void dCol(char delim, char *row, int arg1) {
    int counter = 0;
    int found = 0;
    int j = 0;
    int rowLen = strlen(row);

    for (int i = 0; i < rowLen; i++) {
        if (row[i] == delim) { // checks if delimiter is found
            found++;
        }
        if (found == arg1 - 1) { // the specified column has been found
            while (row[i + 1] != delim && row[i + 1] != '\n' &&
                   row[i + 1] != '\0') { // ignores the letters between do delimiters
                counter++;
                i++;
            }
            i += 1;
            counter++;
            found++;
        }
        if (arg1 != 1) {
            row[j] = row[i];
        } else { row[j] = row[i + 1]; }
        j++;
    }
    for (int i = rowLen - counter; i < rowLen; i++) // removing extra characters that were left in the array
        row[i] = '\0';
}

void setCol(char delim, char *row, int arg1, char *strArg, int selector, bool *warning) {
    char tempArray[MAX_LEN];
    memset(tempArray, '\0', MAX_LEN); // clearing the array in case the function is called multiple times
    int delimSplit = arg1 - 1;
    int counter = 0;
    int rowLen = strlen(row);
    int k = 0;

    for (int i = 0; i < rowLen + 1; i++) {
        if (counter == delimSplit) { // acessing the selected column
            k = i;

            if (strlen(tempArray) + strlen(strArg) <= MAX_LEN - 2) {
                strcat(tempArray, strArg);
            } else {
                return printError(warning, 0, "");
            }

            while (row[k] != delim && row[k] != '\n' && row[k] != '\0') { // ignoring the column that is being replaced
                k++;
            }
            if (selector ==
                1) { // removing delimiter assigned to the column in case of calling from the 'Move' function
                k++;
            }
            for (int j = k; j < rowLen; j++) {
                if (strlen(tempArray) + 1 <= MAX_LEN - 1) {  // segmentation check (-1 cause i want to include \n)
                    tempArray[strlen(tempArray)] = row[j]; // appending the rest of 'row' to the end of the temp array
                } else {
                    return printError(warning, 0, "");
                }
            }
            break;
        }

        if (row[i] == delim) { // accessing the desired column by checking for delimiters
            counter++;
        }
        tempArray[i] = row[i]; // appending characters from row to temp array
    }
    strcpy(row, tempArray); // assigning temp array value back to the row
}

void changeFontSize(char delim, char *row, int arg1, int sizeVal) {
    int delimSplit = arg1 - 1;
    int counter = 0;
    int rowLen = strlen(row);
    int k = 0;

    for (int i = 0; i < rowLen + 1; i++) {
        if (counter == delimSplit) {
            k = i;
            while (row[k] != delim && row[k] != '\n' && row[k] != '\0') { // changes the font size on specific column
                if (sizeVal == 1) {
                    row[k] = toupper(row[k]); // to upper command
                } else { row[k] = tolower(row[k]); } // to lower command
                k++;
            }
        }
        if (row[i] == delim) { // searching for the wanted delimiter
            counter++;
        }
    }
}

void editNum(char delim, char *row, int arg1, int selector,
             bool *warning) { // selector is used for two modes (int and round)
    char tempNum[MAX_LEN];
    memset(tempNum, '\0', MAX_LEN);
    int num = INT_MIN;
    int counter = 0;
    int rowLen = strlen(row);
    float convertedNum = 0;

    for (int i = 0; i < rowLen + 1; i++) {
        if (counter == arg1 - 1) {
            findCol(tempNum, row, delim, arg1); // finds the needed column
            convertedNum = atof(tempNum);
            if (convertedNum != 0) { // if conversion has not failed apply commands
                if (selector == 1) {
                    if (convertedNum > 0) {
                        num = (int) (atof(tempNum) + 0.5f); // int command
                    } else {
                        num = (int) (atof(tempNum) - 0.5f); // round command
                    }
                } else {
                    num = atoi(tempNum); // round command
                }
                break;
            }
        }
        if (row[i] == delim) {
            counter++;
        }
    }
    sprintf(tempNum, "%d", num);
    if (convertedNum != 0) {
        setCol(delim, row, arg1, tempNum, 0, warning);
    }
}

void copyCol(char delim, char *row, int arg1, int arg2, bool *warning, int colCount) {
    if (arg1 != arg2 && colCount + 1 >= arg1 && colCount + 1 >= arg2) {
        char temp[MAX_LEN]; // temp array for column that is beign copied
        memset(temp, '\0', MAX_LEN); // cleaning array before using
        strcpy(temp, findCol(temp, row, delim, arg1)); // coppying the wanted column value to temp array
        setCol(delim, row, arg2, temp, 0, warning); // applying the wanted value to a different column
    }
}

void swapCol(char delim, char *row, int arg1, int arg2, bool *warning, int colCount) {
    if (arg1 != arg2 && colCount + 1 >= arg1 && colCount + 1 >= arg2) {
        char temp[MAX_LEN]; // temp arrays for both columns
        char temp1[MAX_LEN];

        memset(temp, '\0', MAX_LEN);
        memset(temp1, '\0', MAX_LEN);

        strcpy(temp, findCol(temp, row, delim, arg1)); // copying the values to temp arrays
        strcpy(temp1, findCol(temp1, row, delim, arg2));

        setCol(delim, row, arg1, temp1, 0, warning); // applying the values to opposite columns
        setCol(delim, row, arg2, temp, 0, warning);
    }
}

void aCol(char delim, char *row, bool *warning) {
    int rowLen = strlen(row);
    if ((rowLen + 1 <= MAX_LEN - 2)) { // stack overflow check
        char tmpChar = row[rowLen - 1];
        if (tmpChar == '\n') { // adding a delimiter to the line
            row[rowLen - 1] = delim;
            row[rowLen] = '\n';
        } else { // adding a delimiter to the last line without \n
            row[rowLen] = delim;
        }
        row[rowLen + 1] = '\0';
    } else {
        return printError(warning, 0, "");
    }

}

int findMax(int a, int b) { // helper function to determine which arary is larger
    if (a >= b) {
        return a;
    } else {
        return b;
    }
}

void iCol(char delim, char *row, int arg1, bool *warning) {
    int delimNum = 1; // variable used for searching from which delimiter the column should be inserted

    char tempArray[MAX_LEN];
    memset(tempArray, '\0', MAX_LEN);
    int rowLen = strlen(row);

    if (rowLen + 1 <= MAX_LEN - 2) {
        for (int i = 0; i < rowLen; i++) {
            if (arg1 != 1) {
                if (row[i] == delim) { // searching for the specific column
                    delimNum++;
                    if (delimNum == arg1) { // inserting the column
                        strncpy(tempArray, row, i + 1);
                        tempArray[i + 1] = '\0';
                        strcat(tempArray, row + i);
                    }
                }
            } else { // if appending a new col at the start of the row
                tempArray[0] = delim;
                tempArray[1] = '\0';
                strcat(tempArray, row);
            }
        }
        int maxLen = findMax(rowLen, strlen(tempArray)); //getting size of the bigger array

        for (int j = 0; j < maxLen + 1; j++) { // assigning temp array value to the row
            row[j] = tempArray[j];
        }
    } else {
        return printError(warning, 0, "");
    }
}

void iRow(char delim, int arg1, int colCount, int rowNum) { // inserts row before specific a row
    if (arg1 == rowNum) {
        for (int j = 0; j < colCount; j++) {
            printf("%c", delim);
        }
        printf("\n");
    }
}

void moveCol(char delim, char *row, int arg1, int arg2, bool *warning, int colCount) {
    if (arg1 != arg2 && colCount + 1 >= arg1 && colCount + 1 >= arg2) {
        char tempArray[MAX_LEN];
        memset(tempArray, '\0', MAX_LEN);
        if (arg1 >= arg2) { // two different moves for them to work for any arg combination
            strcpy(tempArray, findCol(tempArray, row, delim, arg1)); // copies the column on an index of bigger arg
            setCol(delim, row, arg1, "", 1, warning); // sets a col on the position fo the bigger arg
            iCol(delim, row, arg2, warning); // inserts column next to the column on an index of smaller arg
            setCol(delim, row, arg2, tempArray, 0, warning); // sets the value in temp array to an index of smaller arg
            row[strlen(row) - 1] = '\n';
        } else {
            iCol(delim, row, arg2, warning);
            strcpy(tempArray, findCol(tempArray, row, delim, arg1));
            setCol(delim, row, arg2, tempArray, 0, warning);
            setCol(delim, row, arg1, "", 1, warning);
        }
    }
}

bool isSelected(int arg1, int arg2, int rowNum,
                bool *isLastLine) { // checks if the row is selected by the selection arguments

    if (*isLastLine && arg1 == -1 && arg2 == -1) { // if rows - - used
        return true;
    }

    if (arg2 == -1) { // if rows _ - is used
        if (rowNum >= arg1 && arg2 != arg1) {
            return true;
        }
    }
    if (arg1 != -1) { // if rows - _ is used
        for (int j = arg1; j < arg2 + 1; j++) {
            if (rowNum == j) {
                return true;
            }
        }
    }
    return false;
}

bool beginsWithOrContains(int arg1, char *strArg, char *row, char delim,
                          int selector) { // selector is again used to define which function is used
    char tempArray[MAX_LEN];
    strcpy(tempArray, findCol(tempArray, row, delim, arg1));
    if (selector == 0) { // if command beginswith is used
        if (memcmp(strArg, tempArray, strlen(strArg)) ==
            0) { // checks if array contains strArg characters at the start of it
            return true;
        }
    } else if (selector == 1) { // if command contains is used
        if (strstr(tempArray, strArg) != NULL) { // checks if arrays are the same
            return true;
        }
    }
    return false;
}

int getColCount(char *row, char delim, int rowLen) {
    int counter = 0;
    for (int i = 0; i < rowLen; i++) { // counts the amount of delimiters in each row
        if (row[i] == delim) {
            counter++;
        }
    }
    return counter;
}

void setSelectionArguments(int rowNum, bool *applyOnRow, bool *rowSelection, char delim, char *row, int argId, int arg1,
                           int arg2, char *strArg, bool *isLastLine) { // iterates trough selection arguments
    if (argId == ROWS) { // checking if certain rows were selected using rows command, - - tested only with notepad which did not add newline at the end :(
        *rowSelection = true;
        *applyOnRow = isSelected(arg1, arg2, rowNum, isLastLine);
    } else if (argId == BEGINSWITH) {
        *rowSelection = true;
        *applyOnRow = beginsWithOrContains(arg1, strArg, row, delim, 0);
    } else if (argId == CONTAINS) {
        *rowSelection = true;
        *applyOnRow = beginsWithOrContains(arg1, strArg, row, delim, 1);
    } else { // if no selection command is used then all rows are selected
        if (!*rowSelection) {
            *applyOnRow = true;
        }
    }
}


void setArguments(char delim, char *row, int arg1, int arg2, char *strArg, int rowNum, int argId, int colCount,
                  bool applyOnRow, bool *deleteRow, bool *printLine, bool *warning, bool *isInsertColActive,
                  int *iColCount) {

    switch (argId) { // switch to call arguments that are beign used
        case ACOL:
            aCol(delim, row, warning);
            break;
        case AROW:
            *printLine = true;
            break;
        case DROW:
            *deleteRow = dRow(arg1, arg1, rowNum);
            break;
        case DCOL:
            dCol(delim, row, arg1);
            break;
        case IROW:
            if (isInsertColActive) {
                colCount += *iColCount;
            }
            iRow(delim, arg1, colCount, rowNum);
            break;
        case ICOL:
            if (arg1 <= colCount + 1) {
                iCol(delim, row, arg1, warning);
            } else {
                return printError(warning, 1, "");
            }
            break;
        case DROWS:
            *deleteRow = dRow(arg1, arg2, rowNum);
            break;
        case DCOLS:
            if ((arg2 - arg1 + 1) <= MAX_LEN - 2) {
                for (int k = 0; k < arg2 - arg1 + 1; k++) {
                    dCol(delim, row, arg1);
                }
            } else {
                return printError(warning, 1, "");
            }
            break;
        case CSET:
            if (applyOnRow) {
                setCol(delim, row, arg1, strArg, 0, warning);
            }
            break;
        case TOLOWER:
            if (applyOnRow) {
                changeFontSize(delim, row, arg1, 0);
            }
            break;
        case TOUPPER:
            if (applyOnRow) {
                changeFontSize(delim, row, arg1, 1);
            }
            break;
        case ROUND:
            if (applyOnRow) {
                editNum(delim, row, arg1, 1, warning);
            }
            break;
        case INT:
            if (applyOnRow) {
                editNum(delim, row, arg1, 0, warning);
            }
            break;
        case COPY:
            if (applyOnRow) {
                copyCol(delim, row, arg1, arg2, warning, colCount);
            }
            break;
        case SWAP:
            if (applyOnRow) {
                swapCol(delim, row, arg1, arg2, warning, colCount);
            }
            break;
        case MOVE:
            if (applyOnRow) {
                moveCol(delim, row, arg1, arg2, warning, colCount);
            }
            break;
    }
}

