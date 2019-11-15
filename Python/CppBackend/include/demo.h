#pragma once

#include <iostream>
#include <cstdlib>

/* demo
 * print msgs on the screen
 */
void ThreadInfo(char* msg);

/* demo
 * verify true parallel execution 
 * without Python GIL
 */
int VerifyParallel(int* data);

