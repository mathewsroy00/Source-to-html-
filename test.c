/* ================================================
 * Program  : Calculator
 * Author   : Mathews Roy
 * ================================================ */

#include <stdio.h>
#include <stdlib.h>
#include "calculator.h"

int add(int a, int b)
{
    return a + b;
}

float multiply(float a, float b)
{
    return a * b;
}

int main()
{
    int choice = 0;
    float result = 0.0;
    char operation = '+';

    printf("Welcome to Calculator\n");

    /* taking user input */
    printf("Enter choice : ");
    scanf("%d", &choice);

    if(choice == 1)
    {
        result = add(10, 20);
        printf("Result : %f\n", result);
    }
    else if(choice == 2)
    {
        result = multiply(3.14, 2.0);
        printf("Result : %f\n", result);
    }
    else
    {
        printf("Invalid choice\n");
        return 1;
    }

    return 0;
}