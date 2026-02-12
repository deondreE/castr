#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf("  C Project - CMake + MSVC Build\n");
    printf("===========================================\n\n");
    
    // Test factorial function
    printf("Factorial calculations:\n");
    for (int i = 0; i <= 12; i++) {
        printf("  %2d! = %llu\n", i, factorial(i));
    }
    printf("\n");
    
    // Test prime number function
    printf("Prime numbers up to 100:\n  ");
    int count = 0;
    for (int i = 2; i <= 100; i++) {
        if (is_prime(i)) {
            printf("%3d ", i);
            count++;
            if (count % 10 == 0) printf("\n  ");
        }
    }
    printf("\n\n");
    
    // Test Fibonacci
    printf("Fibonacci sequence (first 15 numbers):\n  ");
    for (int i = 0; i < 15; i++) {
        printf("%llu ", fibonacci(i));
    }
    printf("\n\n");
    
    // Test GCD
    printf("Greatest Common Divisor examples:\n");
    printf("  GCD(48, 18) = %d\n", gcd(48, 18));
    printf("  GCD(100, 35) = %d\n", gcd(100, 35));
    printf("  GCD(17, 19) = %d\n", gcd(17, 19));
    printf("\n");
    
    printf("===========================================\n");
    printf("  Build successful with CMake + MSVC!\n");
    printf("===========================================\n");
    
    return EXIT_SUCCESS;
}