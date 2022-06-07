/** @file sps.c
*
* @brief Chart editation 
* @author Igor Hanus (xhanus19)
*
* @date December 2020 (academic year 2020/2021)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct row {
    char **cells;
    int cellCount;
} Row;

typedef struct tab {
    int rowCount;
    int maxCellNum;
    int maxRowNum;
    Row **rows;
} Tab;

typedef struct selectionArg {
    int minRow;
    int minCol;
    int maxRow;
    int maxCol;
} selectionArg;

typedef struct tempVal {
    char tempVals[10][1001];
    char selTempVal[1001];
}tempValue;

void memoryFree (Tab table);
void inputFormat(Tab *table, int maxCellCount, int maxRowCount,bool *warning);
void readArgs (char *arg, Tab *table, bool *warning);
void tablePrint(Tab *table, FILE *file, char **argv, int j,char delim);
int loadArgs(int argc, char ** argv, char *delim, int *j, bool *multipleDelims, int *argsPos, FILE **file);
int loadTable(Tab *table, FILE **file, int argc, char **argv,int *rowNum, char delim, bool multipleDelims, int *maxCellNum);

int main(int argc, char *argv[]) {
    FILE *file = NULL;
    char delim = ' ';
    bool multipleDelims = false;
    bool warning = false;
    int maxCellNum = 0;
    int rowNum = 0;
    int j = 1;
    int argsPos;
    Tab table;

    int argLoadSuccess = loadArgs(argc,argv,&delim,&j,&multipleDelims,&argsPos,&file); // checks the input arguments
    if (argLoadSuccess != 0) {
        fprintf(stderr, "Arguments load has failed\n");
        return -1;
    }
    int tableLoadSuccess = loadTable(&table,&file,argc,argv,&rowNum,delim,multipleDelims,&maxCellNum); // load the table
    if (tableLoadSuccess != 0) {
        fprintf(stderr, "Table load has failed\n");
        return -1;
    }

    table.maxRowNum = table.rowCount = rowNum+1; //sets the amount of rows
    table.maxCellNum = maxCellNum; // sets the amount of cols/cells
    inputFormat(&table, maxCellNum, rowNum+1, &warning); // resizes the table larger if needed
    if (warning) {
        return  -1;
    }
    readArgs(argv[argsPos], &table, &warning); // reads arguments for table editing
    if (warning) {
        return  -1;
    }
    fclose(file);
    tablePrint(&table,file,argv,j,delim);
    memoryFree(table);
    return 0;
}

void memoryFree (Tab table) { // frees the whole table
    for (int r = 0; r < table.rowCount; r++) {
        for (int q = 0; q < table.rows[r]->cellCount; q++) {
            free(table.rows[r]->cells[q]);
        }
        free(table.rows[r]->cells);
        free(table.rows[r]);
    }
    free(table.rows);
}

void inputFormat(Tab *table, int maxCellCount, int maxRowCount, bool *warning) {
    // first cycle adds new rows if needed, second one adds new cells if needed
    if (table->rowCount <= maxRowCount-1) {
        for (int i = table->rowCount; i < maxRowCount; i++) { // iterates trough every row
            void *tempTab = realloc(table->rows, maxRowCount * sizeof(Row*)); // assigning new row length
            if (tempTab == NULL) {
                memoryFree(*table);
                fprintf(stderr, "Allocation has failed\n");
                *warning = true;
                return;
            }
            table->rows = tempTab;
            table->rows[i] = malloc(sizeof(Row)); // allocates Row
            if (table->rows[i] == NULL) {
                memoryFree(*table);
                fprintf(stderr, "Allocation has failed\n");
                *warning = true;
                return;
            }
            table->rows[i]->cells = malloc( 1 * sizeof(char *));
            if (table->rows[i]->cells == NULL) {
                memoryFree(*table);
                fprintf(stderr, "Allocation has failed\n");
                *warning = true;
                return;
            }
            table->rows[i]->cellCount = 0;
            table->rowCount = maxRowCount;
        }
    }

    for (int r = 0; r < table->rowCount; r++) { // iterates trough rows
        if (table->rows[r]->cellCount < maxCellCount) {
            char **tempCells = realloc(table->rows[r]->cells, maxCellCount * sizeof(char *)); // allocates space for new cells
            if (tempCells == NULL) {
                fprintf(stderr, "Allocation has failed\n");
                *warning = true;
                memoryFree(*table);
                return;
            }
            table->rows[r]->cells = tempCells; // sets allocated space
            int a = table->rows[r]->cellCount; // gets the number of cells before expanding it
            for (int q = a; q < maxCellCount; q++) { // expanding the row with new cells
                table->rows[r]->cells[q] = malloc(2 * sizeof(char));
                if (table->rows[r]->cells[q] == NULL) {
                    fprintf(stderr, "Allocation has failed\n");
                    *warning = true;
                    memoryFree(*table);
                    return;
                }
                strcpy(table->rows[r]->cells[q], ""); // sets an empty cell
            }
            table->rows[r]->cellCount = maxCellCount; // assigns new cell count after expanding the table
        }
    }
}

void tableExpand(int i,Tab *table,selectionArg *selArg, bool *warning){
    if (i == 4) { // used for selections with 4 coordinates
        if (table->maxRowNum < selArg->maxRow && table->maxCellNum < selArg->maxCol) {
            inputFormat(table,selArg->maxCol+1,selArg->maxRow+1,warning); // if both cols and rows need to be added
        } else if (table->maxRowNum < selArg->maxRow) {
            inputFormat(table,table->maxCellNum+1,selArg->maxRow+1,warning); // rows need to be added
        } else {
            inputFormat(table,selArg->maxCol+1,selArg->maxRow +1, warning); // cells need to be added
        }
    } else { // same idea but with selections with 2 coordinates
        if (selArg->maxRow > table->maxRowNum && selArg->maxCol > table->maxCellNum ) {
            inputFormat(table,selArg->maxCol+1,selArg->maxRow+1,warning); //+1 cause it is an index
        } else if (selArg->maxRow > table->maxRowNum) {
            inputFormat(table, table->maxCellNum,selArg->maxRow+1,warning); //+1 cause it is an index
        } else {
            inputFormat(table,selArg->maxCol+1,table->maxRowNum,warning); //+1 cause it is an index
        }

    }
}

// reads the [ ] selections arguments
void setSelectionArgs(selectionArg *selArg,char * arg, Tab *table, bool *warning) {
    int values[1001];
    int i = 0;

    char delim[2] = ",";
    char *tempStr = strtok(arg, delim);

    while( tempStr != NULL ) {

        if (strcmp(tempStr,"_") == 0) { // sets value for '_'
            values[i] = -5000;
        } else if (strcmp(tempStr,"-") == 0) { // sets value for '-'
            if (i % 2 == 0) { // odd position - cells, even - rows
                values[i] = table->maxRowNum;
            } else {
                values[i] = table->maxCellNum ;
            }
        } else {
            values[i] = atoi(tempStr);
        }
        i++;
        tempStr = strtok(NULL, delim);
    }
    if(i == 2) { // sets selections for arguments with two coordinates
        selArg->minRow = selArg->maxRow = values[0]-1; // minrow = maxrow because it is only two coordinate argument
        selArg->minCol = selArg->maxCol = values[1]-1;
        tableExpand(i,table,selArg, warning); // checking if table expansion is needed
    }
    if(i == 4) { // asigning selection for 4 coordinate args
        selArg->minRow = values[0]-1;
        selArg->minCol = values[1]-1;
        selArg->maxRow = values[2]-1;
        selArg->maxCol = values[3]-1;
        tableExpand(i,table,selArg, warning); // checking if table expansion is needed
    }

}

int findNumVal (selectionArg *selArg, Tab *table, int selector) {
    int value = -999999999;
    int coord[2];
    bool firstValidNum = true;
    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == selArg->maxRow || selArg->maxRow == -5001 || (r >= selArg->minRow && r <= selArg->maxRow)) && (q == selArg->maxCol || selArg->maxCol == -5001 || (q >= selArg->minCol && q <= selArg->maxCol))) {
                if (firstValidNum) { // finds first valid number > 0 (if there are no numbers the value 0, same if there were 0s only)
                    if (value < atoi(table->rows[r]->cells[q]) && (atoi(table->rows[r]->cells[q]) != 0)) {
                        value = atoi(table->rows[r]->cells[q]);
                        coord[0] = r; coord[1] = q; //saves the coordinates
                        firstValidNum = false;
                    }
                }
                if (selector == 0) {
                    if (value < atoi(table->rows[r]->cells[q])) { // searched for max number (selector is 0)
                        value = atoi(table->rows[r]->cells[q]);
                        coord[0] = r; coord[1] = q;
                    }
                } else {
                    if ((value > atoi(table->rows[r]->cells[q])) && atoi(table->rows[r]->cells[q]) != 0) {
                        value = atoi(table->rows[r]->cells[q]); // searches for min number (selector > 0)
                        coord[0] = r; coord[1] = q;
                    }
                }
            }
        }
    }
    selArg->maxRow = selArg->minRow = coord[0]; // assigns selection to the chosen number
    selArg->maxCol = selArg->minCol = coord[1];

    return value;
}
// returns the lenght of selected cells
int getStrLen(selectionArg *selArg,Tab *table) {
    int value = 0;

    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == selArg->maxRow || selArg->maxRow == -5001 || (r >= selArg->minRow && r <= selArg->maxRow)) && (q == selArg->maxCol || selArg->maxCol == -5001 || (q >= selArg->minCol && q <= selArg->maxCol))) {
                if(strlen(table->rows[r]->cells[q]) > 0) {
                    value += strlen(table->rows[r]->cells[q]); // increases value by finding selected cells
                }
            }
        }
    }
    return value;
}

// gets the amount of nonempty cells
float getCountVal (selectionArg *selArg,Tab *table) {
    float value = 0.0;

    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == selArg->maxRow || selArg->maxRow == -5001 || (r >= selArg->minRow && r <= selArg->maxRow)) && (q == selArg->maxCol || selArg->maxCol == -5001 || (q >= selArg->minCol && q <= selArg->maxCol))) {
                if(strlen(table->rows[r]->cells[q]) > 0) { // check if cell is empty
                    value ++; //iterates value if cell is not empty
                }
            }
        }
    }
    return value;
}
// getting the sum of selected cells
float getSum (selectionArg *selArg,Tab *table) {
    float value = 0.0;

    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == selArg->maxRow || selArg->maxRow == -5001 || (r >= selArg->minRow && r <= selArg->maxRow)) && (q == selArg->maxCol || selArg->maxCol == -5001 || (q >= selArg->minCol && q <= selArg->maxCol))) {
                value += atof(table->rows[r]->cells[q]); // iterates value of selected cells
            }
        }
    }
    return value;
}
//gets average value from seleted cells
float getAvg (selectionArg *selArg,Tab *table) {
    float value = 0.0;
    float tmpVal = 0.0;
    int divider = 0; // divides with the amount of selected cells

    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == selArg->maxRow || selArg->maxRow == -5001 || (r >= selArg->minRow && r <= selArg->maxRow)) && (q == selArg->maxCol || selArg->maxCol == -5001 || (q >= selArg->minCol && q <= selArg->maxCol))) {
                // testig with isdigit method for some reason did no go trough the tests that were provided in discussion forum

                if (strcmp(table->rows[r]->cells[q], "0") != 0){ // if string is not zero (exception fo the number)
                    tmpVal = atof(table->rows[r]->cells[q]);
                } else { // if it is zero then increment
                    divider ++;
                    tmpVal = 0;
                }

                if (tmpVal != 0) {
                    value += atof(table->rows[r]->cells[q]);
                    divider++;
                }
            }
        }
    }
    if (divider != 0) {
        return value/divider;
    } else {
        return value;
    };
}

char * trimArg (char * arg) { // gets rid of [] brackets
    int tempPos = 0;
    static char tempArg[1000];

    memcpy(tempArg, arg, strlen(arg));
    if (arg[0] != '[') { // check if it's a argument with [] brackets
        return "err";
    }

    for (int i = 1; i < (signed)strlen(arg)-1; i++) {
        tempArg[tempPos] = arg[i]; // loads the content in [ ] to tempArg
        tempPos++;
    }
    tempArg[tempPos] = '\0';

    return tempArg;
}

// finds a substring from defined index
char * findSubstring (char * string, int index, int selector) {
    int j = 0;
    static char tmp[1000];
    memset(tmp,'\0',1000 * sizeof(char));
    memcpy(tmp, string, strlen(string)); // copies string to temp
    for (int i = index+1; i < (signed) strlen(string); i++) {
        tmp[j] = tmp[i]; // loading characters from a certain index
        j++;
    }
    if (selector == 1) { // slsector is 1 for ending the string and 0 for cutting one character off
        tmp[j] = '\0';
    } else {
        tmp[j-1] = '\0';
    }
    return tmp;
}

void freeRow(Row *row) { // helper function to free seleted row
    for (int i = 0; i < row->cellCount; i++) {
        free(row->cells[i]);
    }
    free(row->cells);
    free(row);
}

// deletes the selected row
void dRow(selectionArg *selArg, Tab *table) {
    int destRow = 0;
    for (int i = 0; i < table->rowCount; i++) {
        if (i < selArg->minRow || i > selArg->maxRow) { // condition for checking rows that are not selected
            table->rows[destRow] = table->rows[i];
            destRow++;
        } else { // freeing the selected row
            freeRow(table->rows[i]);
        }
    }
    table->rowCount = destRow; // setting the current row count
}

// helper function fo freeing cell
void freeCell(Row *row, int q) {
    free(row->cells[q]);
}

// same idea a drow but also iterating trough cells
void dCol(selectionArg *selArg, Tab *table) {
    int desctCell = 0;

    for (int r = 0; r < table->rowCount; r++) { // iterating trough rows
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if (q < selArg->minCol || q > selArg->maxCol) {
                table->rows[r]->cells[desctCell] = table->rows[r]->cells[q];
                desctCell++;
            } else {
                freeCell(table->rows[r],q); // freeing selected cell
            }
        }
        table->rows[r]->cellCount = desctCell; // changing the amount of cells in a row
        table->maxCellNum = desctCell;
        desctCell = 0; // resetting the value for next iteration
    }

}

void insertCol(selectionArg *selArg, Tab *table, int selector, bool *warning) {
    inputFormat(table,table->maxCellNum+1, table->maxRowNum, warning); // increasing size of table by one
    int incVal = 0;
    if (selector == 1) { //for icol
        incVal = selArg->maxCol +1;
    }else { //for acol
        incVal = selArg->maxCol;
    }

    for (int r = 0; r < table->rowCount; r++) {
        for (int q = table->maxCellNum;q > incVal; q--) { // iterates trough cells from the end and swaps their order
            char * tmpSwapCell  = table->rows[r]->cells[q]; // cell on position i is swapped with the one on position i-1
            table->rows[r]->cells[q] = table->rows[r]->cells[q-1];
            table->rows[r]->cells[q-1] = tmpSwapCell;
        }
    }
    table->maxCellNum = table->maxCellNum+1; // setting the new cell count
}

void insertRow(selectionArg *selArg, Tab *table, int selector, bool *warning) {
    inputFormat(table,table->maxCellNum, table->maxRowNum+1, warning); // adding one row to the table
    Row * tmpRow = table->rows[table->maxRowNum]; // setting temp values from rows to be swapped
    Row * tmpSwappedRow = table->rows[selArg->minRow];

    table->rows[table->maxRowNum] = tmpSwappedRow; // swapping initial temp values
    table->rows[selArg->minRow] = tmpRow;
    int initVal = selArg->maxRow ;
    if (selector == 1) { // again used for differetiating irow/arow
        initVal--;
    }
    for(int i = initVal+1; i < table->maxRowNum; i++) { // swapping rows
        tmpRow = table->rows[table->maxRowNum + initVal - (i-1)];
        tmpSwappedRow = table->rows[table->maxRowNum + initVal - i];
        table->rows[table->maxRowNum + initVal - (i-1)] = tmpSwappedRow;
        table->rows[table->maxRowNum + initVal - i] = tmpRow;
    }

}

void setTmpCoords(char * arg, int *r, int *c) { // creating a temporary selection
    char tmp[1001];
    int tmpPos = 0;
    bool coordSwitch = false;
    if (strlen(arg) < 1001) {
        for (int i = 0; i < (signed) strlen(arg); i++) {
            if (arg[i] == ',' || i == (signed) strlen(arg) -1) { // check if the , divider is on current index
                if (!coordSwitch) { // checks if the second coordinate is being processed
                    tmp[i] = '\0';
                } else {
                    tmp[tmpPos] = arg[i];
                    tmp[tmpPos+1] = '\0';
                    *c = atoi(tmp)-1; // converting to int
                    break;
                }
                tmpPos = 0;
                i++;

                coordSwitch = true;
                *r = atoi(tmp)-1; // sets row value and turn coordswitch to true to check column value
            }
            tmp[tmpPos] = arg[i]; // loads number in tmp
            tmpPos++;
        }
    }
    tmp[tmpPos+1] = '\0';
    *c = atoi(tmp)-1; // the column value at the end of the [ ] argument
}

void setCellValue(char * value, int *row, int *col,int *mRow, int *mCol, Tab *table, bool *warning) {
    inputFormat(table,*mCol,*mRow, warning);

    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == *mRow || *mRow == -5001 || (r >= *row && r <= *mRow)) && (q == *mCol || *mCol == -5001 || (q >= *col && q <= *mCol))) {
                void *temp = realloc(table->rows[r]->cells[q], strlen(value)+1 * sizeof(char)); // adjusts the cell to needed size
                if (temp == NULL) {
                    memoryFree(*table);
                    fprintf(stderr,"Allocation has failed");
                    *warning = true;
                    return;
                }
                table->rows[r]->cells[q] = temp; // sets the realloc
                strcpy(table->rows[r]->cells[q], value); // sets the value
            }
        }
    }
}

void incTempVal(char *arg,tempValue *tmpVal, bool * warning) { // incrementing temp value on chosen index
    int tempValNum = atoi(findSubstring(arg, 4,1)); //index
    if (tempValNum < 10) {
        int incVal =  atoi(tmpVal->tempVals[tempValNum])+1; //if 0 then set it to 1
        char buffer[1000];
        memset(buffer,'\0',1000);
        snprintf(buffer, 2 *sizeof(int), "%d", incVal); // turning it into string
        strcpy( tmpVal->tempVals[tempValNum],buffer);
    } else {
        fprintf(stderr, "Temp value called outside the bounds.\n");
        *warning = true;
        return;
    }
}

void setTempVal(char *arg,tempValue *tmpVal,selectionArg *selArg,Tab *table, bool *warning) {
    int tempValNum = atoi(findSubstring(arg, 4,1));
    if (tempValNum < 10) { // sets temp value onm chosen cells
        setCellValue(tmpVal->tempVals[tempValNum],&selArg->minRow,&selArg->minCol, &selArg->maxRow, &selArg->maxCol,table, warning);

    } else {
        fprintf(stderr, "Temp val number is over allowed limit.\n");
        *warning = true;
        return;
    }
}


void defTempVal(char *arg,tempValue *tmpVal,selectionArg *selArg,Tab *table, bool *warning) {
    int tempValNum = atoi(findSubstring(arg, 4,1));
    if (tempValNum < 10) { // defines temp value
        strcpy(tmpVal->tempVals[tempValNum],table->rows[selArg->maxRow]->cells[selArg->maxCol]);
    } else {
        fprintf(stderr, "Temp val number is over allowed limit.\n");
        *warning = true;
        return;
    }
}

//sumCells(buffer,arg,table, 0);
void sumCells (char *val,char * arg , Tab *table, int selector, bool *warning) {
    int r = 0;
    int c = 0;
    char *tmp = findSubstring(arg, 4,0);
    if (selector == 1) {
        tmp = findSubstring(arg, 6,0);
    }
    setTmpCoords(tmp, &r, &c); //returns indexes
    // choosing which property of the table should be enxpanded (if there is +1 then it is being expanded)
    if (r > table->maxRowNum && c > table->maxCellNum ) {
        inputFormat(table,c+1,r+1, warning); //+1 cause it is an index
    } else if (r > table->maxRowNum) {
        inputFormat(table, table->maxCellNum,r+1, warning); //+1 cause it is an index
    } else {
        inputFormat(table,c+1,table->maxRowNum, warning); //+1 cause it is an index
    }
    setCellValue(val,&r,&c,&r,&c,table, warning); // sets the value
}

void swapCells (selectionArg *selArg, char * arg , Tab *table, bool *warning) {
    int r = 0;
    int c = 0;
    char * tmp = findSubstring(arg, 5,0); //getting the coordinates
    setTmpCoords(tmp, &r, &c); //setting tmp coordinates to knwo where to put the value

    char *tmpCell = table->rows[r]->cells[c]; // gets the dest cell value
    int rowSel = selArg->minRow;
    int minCol = selArg->minCol;
    if (selArg->minRow == -5001) {
        rowSel = 0;
    }
    char *tmpCellSwap = malloc(strlen(table->rows[rowSel]->cells[minCol])+1);

    if (tmpCellSwap == NULL) {
        *warning = true;
        fprintf(stderr, "Allocation has failed.\n");
        memoryFree(*table);
        return;
    }
    strcpy(tmpCellSwap, table->rows[rowSel]->cells[minCol]); // gets the value to be swapped

    setCellValue(tmpCell,&rowSel,&minCol,&rowSel,&minCol,table, warning); // swaps the values
    setCellValue(tmpCellSwap,&r,&c,&r,&c,table, warning);
    free(tmpCellSwap);
}

void findStringVal(selectionArg *selArg, char * arg , Tab *table) { // finds cell by string
    char * tmp = findSubstring(arg, 5,0);
    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == selArg->maxRow || selArg->maxRow == -5001 || (r >= selArg->minRow && r <= selArg->maxRow)) && (q == selArg->maxCol || selArg->maxCol == -5001 || (q >= selArg->minCol && q <= selArg->maxCol))) {
                if(strcmp(table->rows[r]->cells[q], tmp) == 0) { // compares if string in cells is the same as the searched one
                    selArg->maxRow = selArg->minRow = r;
                    selArg->maxCol = selArg->minCol = q;
                    break;
                }
            }
        }

    }
}

void setPos(char * value,selectionArg *selArg, Tab *table, bool * warning) { // sets value to a chosen cell
    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if ((r == selArg->maxRow || selArg->maxRow == -5001 || (r >= selArg->minRow && r <= selArg->maxRow)) && (q == selArg->maxCol || selArg->maxCol == -5001 || (q >= selArg->minCol && q <= selArg->maxCol))) {
                if(strlen(table->rows[r]->cells[q])+1 < strlen(value)+1) {
                    void *temp = realloc(table->rows[r]->cells[q], strlen(value)+1 * sizeof(char));
                    if (temp == NULL) {
                        *warning = true;
                        memoryFree(*table);
                        fprintf(stderr, "Allocation has failed in setPos function");
                        return;
                    }
                    table->rows[r]->cells[q] = temp;
                }
                strcpy(table->rows[r]->cells[q], value);
            }
        }

    }
}

void useTempSelection(tempValue *tmpVal, selectionArg *selArg, Tab *table, bool *warning) { // uses temp selection coordinates
    setSelectionArgs(selArg,trimArg(tmpVal->selTempVal),table, warning);
}

void setTempSelection(selectionArg *selArg, tempValue *tmpVal) {
    char buffer[1000];

    if (selArg->maxRow == selArg->minRow && selArg->minCol == selArg->maxCol) {
        snprintf(buffer, 1000, "[%d,%d]", selArg->maxRow+1,selArg->maxCol+1); // if temp selection has two values
    } else {
        snprintf(buffer, 1000, "[%d,%d,%d,%d]", selArg->minRow+1,selArg->minCol+1,selArg->maxRow+1,selArg->maxCol+1); // if tmp selection has four values
    }
    strcpy(tmpVal->selTempVal,buffer);
}

void processArg (selectionArg *selArg,tempValue *tmpVal, char *arg, Tab *table, bool * warning) {
    char temp[1001]; // used for set STR value
    int pos = 0; // used for iterating trough set STR value
    char *tmpArg = trimArg(arg); // in case of args in [ ] brackets, the brackets get removed
    if (strlen(arg) < 1001) {
        // set argument
        if(strncmp(arg, "set", 3) == 0 && memcmp(arg,"set _",5) != 0) {
            for (int i = 4; i < (signed) strlen(arg) +1; i++) { // gets all cahracters after space in 'set' command
                temp[i-4] = arg[i];
                pos = i-4;
            }
            temp[pos] ='\0';
            setPos(temp, selArg, table, warning);
        }
        // TABLE SELECTION ARGS
        else if (strcmp(tmpArg,"_") == 0) {
            useTempSelection(tmpVal, selArg, table, warning);
        } else if (strcmp(tmpArg,"set") == 0) {
            setTempSelection(selArg, tmpVal);
        } else if (strcmp(tmpArg,"max") == 0) {
            findNumVal(selArg, table, 0);
        } else if (strcmp(tmpArg,"min") == 0) {
            findNumVal(selArg, table, 1);
        } else if (memcmp(tmpArg,"find",4) == 0) {
            findStringVal(selArg, arg, table);
        }
        // TABLE STRUCTURE EDIT ARGS
        else if (strcmp(arg,"irow") == 0) {
            insertRow(selArg, table, 0,warning);
        } else if (strcmp(arg,"arow") == 0) {
            insertRow(selArg, table, 1, warning);
        } else if (strcmp(arg,"drow") == 0) {
            dRow(selArg, table);
        } else if (strcmp(arg,"icol") == 0) {
            insertCol(selArg, table, 0, warning);
        } else if (strcmp(arg,"acol") == 0) {
            insertCol(selArg, table, 1, warning);
        } else if (strcmp(arg,"dcol") == 0) {
            dCol(selArg, table);
        }
        // TABLE CONTENT ARGUMENTS
        else if (strcmp(arg,"clear") == 0) {
            setPos("", selArg, table, warning);
        } else if (memcmp(arg,"swap",4) == 0) {
            swapCells(selArg, arg, table, warning);
        } else if (memcmp(arg,"sum",3) == 0) {
            double sumVal = getSum(selArg, table);
            char buffer[50];
            snprintf(buffer, 2 *sizeof(double), "%g", sumVal); // sets string to %g format
            sumCells(buffer,arg,table, 0, warning);
        } else if (memcmp(arg,"avg",3) == 0) {
            double avgVal = getAvg(selArg, table);
            char buffer[50];
            snprintf(buffer, 2 *sizeof(double), "%g", avgVal); // sets string to %g format
            sumCells(buffer,arg,table, 0, warning);
        } else if (memcmp(arg,"count",5) == 0) {
            double countVal = getCountVal(selArg, table);
            char buffer[50];
            snprintf(buffer, 2 *sizeof(double), "%g", countVal); // sets string to %g format
            sumCells(buffer,arg,table, 1, warning);
        } else if (memcmp(arg,"len",3) == 0) {
            int countVal = getStrLen(selArg, table);
            char buffer[50];
            snprintf(buffer, 2 *sizeof(int), "%d", countVal); // sets string to %d format
        }
        // TEMP VALUES ARGUMENTS
        else if (memcmp(arg,"def",3) == 0) { // def _X arg
            defTempVal(arg, tmpVal, selArg, table, warning);
        } else if (memcmp(arg,"use",3) == 0) { // use _X arg
            setTempVal(arg, tmpVal, selArg, table, warning);
        } else if (memcmp(arg,"inc",3) == 0) { // inc _X arg
            incTempVal(arg, tmpVal, warning);
        }
    } else {
        fprintf(stderr,"Argument is too long\n");
        *warning = true;
        return;
    }
}



void readArgs (char *arg, Tab *table, bool * warning) {
    selectionArg selArg; //initializing selection args
    selArg.minRow = 0;
    selArg.minCol = 0;
    selArg.maxCol = 0;
    selArg.maxRow = 0;

    tempValue tmpVal;

    for (int i = 0; i < 10;i++) {
        strcpy(tmpVal.tempVals[i] ,"0"); //initializing temp vals
    }

    char argList [1001][1001];
    int i = 0;

    char delim[2] = ";";
    char *tempStr = strtok(arg, delim);

    while( tempStr != NULL ) {
        // setting cell selection
        strcpy(argList[i], tempStr);
        i++;
        tempStr = strtok(NULL, delim);
    }

    for (int j = 0; j < i; j++) {
        if (argList[j][0] == '[' && strcmp(trimArg(&argList[j][0]),"max") != 0 && strcmp(trimArg(&argList[j][0]),"min") != 0 ) { // checks if selection arguments with coordinates is used
            setSelectionArgs(&selArg, trimArg(&argList[j][0]), table, warning);
        }
        processArg(&selArg, &tmpVal, &argList[j][0], table, warning);
    }

}

void tablePrint(Tab *table, FILE *file, char **argv, int j,char delim) {
    int tmpPrintAMount =0;
    int cellPrintAmount = table->rows[0]->cellCount;
    for (int r = 0; r < table->rowCount; r++) {
        for (int q = table->rows[r]->cellCount -1; q > -1; q--) {
            if (strcmp(table->rows[r]->cells[q],"") == 0) { // gettinf the amound of empty cells at the end of the row
                tmpPrintAMount++;
            }
        }
        if (tmpPrintAMount < cellPrintAmount) {
            cellPrintAmount = tmpPrintAMount; // getting the minimum amount of cells that need to be deleted
        }
        tmpPrintAMount = 0;
    }

    file = fopen(argv[j + 1], "w");
    for (int r = 0; r < table->rowCount; r++) {
        for (int q = 0; q < table->rows[r]->cellCount; q++) {
            if (strcmp(table->rows[r]->cells[q], " ") != 0) {
                bool cointainsDelim = false;
                if(strchr(table->rows[r]->cells[q], delim) != NULL ||strchr(table->rows[r]->cells[q], ' ') != NULL ) {
                    fprintf(file,"\""); // use " " if cell contains delim or whitespace
                    cointainsDelim = true;
                }

                if (q == table->rows[0]->cellCount-1) {
                    if (cointainsDelim) { // id the cell contains delim and it is the last cell put it in " "
                        fprintf(file,"%s\"", table->rows[r]->cells[q]);
                    } else {
                        fprintf(file ,"%s", table->rows[r]->cells[q]);
                    }
                } else {
                    if (cointainsDelim) { // if cell contains delim put iit in " "
                        fprintf(file ,"%s\"%c", table->rows[r]->cells[q],delim);
                    } else {
                        fprintf(file ,"%s%c", table->rows[r]->cells[q],delim);
                    }
                }
            }
        }
        fprintf(file,"\n");
    }
    fclose(file);
}

int loadArgs(int argc, char ** argv, char *delim, int *j, bool *multipleDelims, int *argsPos, FILE **file) {
    if (argc > 1) {
        if (strcmp(argv[1], "-d") == 0) { // checks if delim is set
            *delim = argv[2][0]; // sets primary delim
            *j = 3; // if delim is used then ags field is at index 3
            if (strlen(argv[2]) > 1) {
                *multipleDelims = true;
            }
        }
        *argsPos = *j;
        if (argc > 2) { // checks if there's more than 2 args
            *file = fopen(argv[*j + 1], "r");
            if (*file == NULL) {
                fprintf(stderr, "File does not exist\n");
                return -1;
            }
        } else { // file nad args arguments are mandatory so if they are not use exception is thrown
            fprintf(stderr, "Incorrect input\n");
            return -1;
        }
    }
    else {
        fprintf(stderr, "Incorrect input\n");
        return -1;
    }
    return 0;
}

int loadTable(Tab *table, FILE **file, int argc, char **argv,int *rowNum, char delim, bool multipleDelims, int *maxCellNum) {
    bool ignoreDelim = false;
    bool incrementCell = true;
    bool isSpecialSymbol = false;
    int incrementnext = 0;

    int cellNum = 0;
    int rowsCap = 10;
    int cellsCap = 10;
    int cellCap = 10;
    int i = 0;
    int input;

    table->rows = malloc(rowsCap * sizeof(Row*)); // creating a pointer tp rpw
    table->rows[0] = malloc(sizeof(Row));
    table->rows[*rowNum]->cells = malloc(cellsCap * sizeof(char*)); // creating a pointer to string array

    table->rows[0]->cellCount = 0;

    char *cell = malloc(cellCap * sizeof(char));
    if (table->rows == NULL || table->rows[0] == NULL || table->rows[*rowNum]->cells == NULL) {
        memoryFree(*table);
        fclose(*file);
        return -1;
    }

    if (argc > 2 && file != NULL) {
        while ((input = fgetc(*file)) != EOF) {

            if (isSpecialSymbol) {
                incrementnext ++;
            }

            if (input == '\\' && !ignoreDelim) { //if / is used dont use the next symbol as 'special'
                isSpecialSymbol = true;

                incrementCell = false;

            } else if (input == '"' ) { //if " is used ignore delimiter until another " appears
                if (ignoreDelim) {
                    ignoreDelim = false;
                }
                else {
                    ignoreDelim = true;
                }
                incrementCell = false;
            } else {
                cell[i] = (char) input; // add value to temporary cell
                incrementCell = true;
            }


            if (i == cellCap-1) { // if cellCap size is reached realloc the size
                cellCap *= 2;
                char * tempCell = realloc(cell, cellCap * sizeof(char));
                if (tempCell == NULL) {
                    memoryFree(*table);
                    fclose(*file);
                    return -1;
                }
                cell = tempCell;
            }

            if (*rowNum == rowsCap-1) { // if last row number is reached realloc the size
                rowsCap *= 2;
                void *tempTab = realloc(table->rows, rowsCap * sizeof(Row*));
                if (tempTab == NULL) {
                    memoryFree(*table);
                    fclose(*file);
                    return -1;
                }
                table->rows = tempTab;
            }

            // saving the cell after reaching the delimiter or end of the line
            if ((cell[i] == delim || cell[i] == '\n' || (multipleDelims && strchr(argv[2],cell[i]))) && !ignoreDelim && !isSpecialSymbol) {
                cell[i] = '\0';
                if (cellNum == cellsCap) {
                    cellsCap *= 2;
                    char **tempCells = realloc(table->rows[*rowNum]->cells, cellsCap * sizeof(char *)); //allocating space needed for temp cell

                    if (tempCells == NULL) {
                        memoryFree(*table);
                        fclose(*file);
                        return -1;
                    }
                    table->rows[*rowNum]->cells = tempCells;
                }

                table->rows[*rowNum]->cells[cellNum] = malloc((strlen(cell) + 1) * sizeof(char));
                if (table->rows[*rowNum]->cells[cellNum] == NULL) {
                    memoryFree(*table);
                    fclose(*file);
                    return -1;
                }
                strcpy(table->rows[*rowNum]->cells[cellNum], cell); // assigning the temp cell to a cell

                cellNum++;
                i = -1;
            }

            if (input == '\n') {
                int tempChar = fgetc(*file); // checks if next character is EOF
                ungetc(tempChar, *file);

                if (cellNum > *maxCellNum) {
                    *maxCellNum = cellNum;
                }

                table->rows[*rowNum]->cellCount = cellNum;
                if (tempChar != EOF) { // if next character is not EOF allocate space for the next row
                    (*rowNum)++;
                    table->rows[*rowNum] = malloc(sizeof(Row));
                    table->rows[*rowNum]->cells = malloc(cellsCap * sizeof(char *));
                    if (table->rows[*rowNum]->cells == NULL || table->rows[*rowNum] == NULL) {
                        memoryFree(*table);
                        fclose(*file);
                        return -1;
                    }
                }
                cellNum = 0; // resetting cellNum and cellCap
                cellCap = 10;
            }

            if (incrementnext == 1) {
                isSpecialSymbol = false;
                incrementnext = 0;
            }

            if (incrementCell) {
                i++;
            }
        }

    }
    free(cell);
    return 0;
}

