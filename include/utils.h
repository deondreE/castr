#ifndef UTILS_H
#define UTILS_H

/**
 * Calculate the factorial of n
 * @param n The number to calculate factorial for
 * @return n! (factorial of n)
 */
unsigned long long factorial(int n);

/**
 * Check if a number is prime
 * @param n The number to check
 * @return 1 if prime, 0 otherwise
 */
int is_prime(int n);

/**
 * Calculate the nth Fibonacci number
 * @param n The position in the Fibonacci sequence (0-indexed)
 * @return The nth Fibonacci number
 */
unsigned long long fibonacci(int n);

/**
 * Calculate the Greatest Common Divisor of two numbers
 * @param a First number
 * @param b Second number
 * @return GCD of a and b
 */
int gcd(int a, int b);

#endif // UTILS_H