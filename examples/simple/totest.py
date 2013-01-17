def factorial(n):
  if n <= 1:
    return 1
  return factorial (n - 1) * n

def fibonacci(n):
  if n == 0 or n == 1:  
    return n
  else:
    return fibonacci (n - 1) + fibonacci (n - 2)
