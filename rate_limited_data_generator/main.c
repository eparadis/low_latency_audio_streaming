/*
Name: rate_limited_data_generator

This program produces arbitrary patterns of data to stdout at a specified rate until closed with CTRL+C.

It takes the following command line options, all of which are optional.

  -r N              Rate at which to send output. The defalt is 8khz.
  -b N M            Send an incrememnting signed 32bit value that starts at N. When it reaches M, it will begin decrementing
                    back towards N. Once at N, it will begin incrementing again, producing a "triangle wave" of values.
                    The default is -128 to 128
  -c N              Send N values and then terminate. Negative values are treated as "forever". This is default behavior.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

void sig_handler(int signo)
{
  if (signo == SIGINT)
  {
    printf("\nCTRL+C pressed. Exiting...\n");
    exit(0);
  }
}

int main(int argc, char *argv[])
{
  int c;
  int rate = 8000; // Default rate
  int start_value = -128;
  int end_value = 128;
  int current_value = start_value;
  int count_limit = -1;       // -1 means no limit
  int count = 0;
  int incrementing = 1; // Default to incrementing
  int verbose = 0;
  struct timeval start_time, end_time, elapsed_time;

  // Parse command line arguments
  while ((c = getopt(argc, argv, "r:b:c:")) != -1)
  {
    switch (c)
    {
    case 'r':
      rate = atoi(optarg);
      break;
    case 'b':
      sscanf(optarg, "%d %d", &start_value, &end_value);
      if (start_value == end_value) {
        fprintf(stderr, "start and end values must differ.\n");
        exit(EXIT_FAILURE);
      }
      current_value = start_value;
      break;
    case 'c':
      count_limit = atoi(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      fprintf(stderr, "Usage: %s [-r N] [-b N M] [-c N] [-v]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // Set up signal handler for CTRL+C
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  // Initialize start time
  gettimeofday(&start_time, NULL);

  // Main loop
  while (count < count_limit || count_limit == -1)
  {
    printf("%d\n", current_value);
    fflush(stdout); // Ensure output is flushed immediately
    count++;

    if (current_value == end_value)
      incrementing = 0; // Switch to decrementing
    if (current_value == start_value)
      incrementing = 1; // Switch to incrementing

    if( incrementing == 1) {
      current_value++;
    } else {
      current_value--;
    } 

    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &elapsed_time);
    long elapsed_micros = (elapsed_time.tv_sec * 1000000) + (elapsed_time.tv_usec);
    long target_time_micros = 1000000 / rate; // Target time in microseconds
    long sleep_time_micros = target_time_micros - elapsed_micros;
    if (sleep_time_micros > 0)
    {
      if (verbose)
        printf("rate: %d Hz  target time: %ld us  elapsed time: %ld us  sleep time: %ld us\n", rate, target_time_micros, elapsed_micros, sleep_time_micros);
      usleep(sleep_time_micros); // Sleep for the remaining time
    }
    else
    {
      // If the sleep time is negative, we are behind schedule
      fprintf(stderr, "Warning: Overrun detected. Adjusting sleep time.\n");
    }
    // Reset start time for the next iteration
    gettimeofday(&start_time, NULL);

  }
  return 0;
}
