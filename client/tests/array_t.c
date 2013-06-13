/*
  The following are tests for the file array.c
*/
#ifndef ARRAY_TESTS
#define ARRAY_TESTS

#include <stdio.h>
#include <math.h>

int floats_nearly_equal(float flt1, float flt2)
{
  /* 
     Added because floating point precision is a little off. Returns whether
     two floats are practically equal.
  */
  if (fabs((flt1 + 0.00001)/flt2) > 0.950) {
    return 1;
  }
  return 0;
}

int bytes_equal(uint8_t *array1, int start1, uint8_t *array2, int start2, 
		int len) {
  while ((abs(start1 - len) > 0) && (abs(start2 - len) > 0)) {
    if (array1[start1] != array2[start2]) {
      return 0;
    }
    start1++;
    start2++;
  }
  return 1;
}

int array_test(void)
{
  int i;

  int tests_passed;
  int tests_failed;

  uint8_t flt_bytes[4];
  float flt;

  uint8_t shrt_bytes[2];
  short shrt;

  uint8_t flt_bytes_exp[4];

  tests_passed = 0;
  tests_failed = 0;

  /* failed_tests = NULL; */
  /* failed_tests_exp = NULL; */
  /* failed_tests_got = NULL; */

  /*
    Testing float_from_byte_array
  */
  
  /* 0 */
  for (i = 0; i < 4; i++) { flt_bytes[i] = 0; }
  flt = float_from_byte_array(flt_bytes, 0);
  
  if (floats_nearly_equal(flt, (float) 0.00001)) {
    tests_passed++;
  }
  else {
    printf("Test failed: float_from_byte_array(0 case)\n");
    printf("Expected: 0.00000\n");
    printf("Got: %f\n", flt);
    tests_failed++;
  }

  /* 1: test case 0x3F800000 == 1 */
  flt_bytes[0] = 0x00;
  flt_bytes[1] = 0x00;
  flt_bytes[2] = 0x80;
  flt_bytes[3] = 0x3F;

  flt = float_from_byte_array(flt_bytes, 0);
  if (floats_nearly_equal(flt, (float) 1)) {
    tests_passed++;
  }
  else {
    printf("Test failed: float_from_byte_array(1 case)\n");
    printf("Expected: 1\n");
    printf("Got: %f\n", flt);
    tests_failed++;
  }
 
  /* inf: test case 0x47C35000 == 100000 */
  flt_bytes[0] = 0x00;
  flt_bytes[1] = 0x05;
  flt_bytes[2] = 0xC3;
  flt_bytes[3] = 0x47;

  flt = float_from_byte_array(flt_bytes, 0);
  if (floats_nearly_equal(flt, (float) 100000)) {
    tests_passed++;
  }
  else {
    printf("Test failed: float_from_byte_array(inf case)\n");
    printf("Expected: 100000\n");
    printf("Got: %f\n", flt);
    tests_failed++;
  }

  /*
    Testing short from byte_array
  */

  /* 0 */
  shrt_bytes[0] = 0; shrt_bytes[1] = 0;
  
  shrt = short_from_byte_array(shrt_bytes, 0);
  if (shrt == 0) {
    tests_passed++;
  }
  else {
    printf("Test failed: short_from_byte_array(0 case)\n");
    printf("Expected: 0\n");
    printf("Got: %d\n", shrt);
    tests_failed++;
  }

  /* 1 */
  shrt_bytes[0] = 0x01; shrt_bytes[1] = 0x00;

  shrt = short_from_byte_array(shrt_bytes, 0);
  if (shrt == 1) {
    tests_passed++;
  }
  else {
    printf("Test failed: short_from_byte_array(1 case)\n");
    printf("Expected: 1\n");
    printf("Got: %d\n", shrt);
    tests_failed++;
  }

  /* inf 0x7FFF == 32767 */
  shrt_bytes[0] = 0xFF; shrt_bytes[1] = 0x7F;
  
  shrt = short_from_byte_array(shrt_bytes, 0);
  if (shrt == 0x7FFF) {
    tests_passed++;
  }
  else {
    printf("Test failed: short_from_byte_array(inf case)\n");
    printf("Expected: 32767 \n");
    printf("Got: %d\n", shrt);
    tests_failed++;
  }

  /*
    Testing float_into_byte_array
  */

  /* 0 */
  flt = (float) 0;
  
  float_into_byte_array(flt_bytes, 0, flt);
  flt_bytes_exp[0] = 0; flt_bytes_exp[1] = 0; 
  flt_bytes_exp[2] = 0; flt_bytes_exp[3] = 0; 

  if (bytes_equal(flt_bytes, 0, flt_bytes_exp, 0, 4)) {
    tests_passed++;
  }
  else {
    printf("Test failed: float_into_byte_array(0 case)\n");
    printf("Expected: {0, 0, 0, 0}\n");
    printf("Got: {%d, %d, %d, %d}\n", 
	    flt_bytes[0], flt_bytes[1], flt_bytes[2], flt_bytes[3]);
    tests_failed++;
  }

  /* 1 */ 
  flt = (float) 1;
  
  float_into_byte_array(flt_bytes, 0, flt);
  flt_bytes_exp[0] = 0x00; flt_bytes_exp[1] = 0x00; 
  flt_bytes_exp[2] = 0x80; flt_bytes_exp[3] = 0x3F;

  if (bytes_equal(flt_bytes, 0, flt_bytes_exp, 0, 4)) {
    tests_passed++;
  }
  else {
    printf("Test failed: float_into_byte_array(1 case)\n");
    printf("Expected: {0, 0, 128, 63}\n");
    printf("Got: {%d, %d, %d, %d}\n", 
	    flt_bytes[0], flt_bytes[1], flt_bytes[2], flt_bytes[3]);
    tests_failed++;
  }

  /* inf */
  flt = (float) 100000;
  
  float_into_byte_array(flt_bytes, 0, flt);
  flt_bytes_exp[0] = 0; flt_bytes_exp[1] = 80; 
  flt_bytes_exp[2] = 195; flt_bytes_exp[3] = 71; 

  if (bytes_equal(flt_bytes, 0, flt_bytes_exp, 0, 4)) {
    tests_passed++;
  }
  else {
    printf("Test failed: float_into_byte_array(inf case)\n");
    printf("Expected: {0, 80, 195, 71}\n");
    printf("Got: {%d, %d, %d, %d}\n", 
	   flt_bytes[0], flt_bytes[1], flt_bytes[2], flt_bytes[3]);
    tests_failed++;
  }

  return 0;
}

#endif
