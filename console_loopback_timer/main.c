/*
Name: console_loopback_timer

This program receives a line of text on stdin and prints it unchanged to stdout until closed with CTRL+C.
With no arguments, for each line it receives, it prints the number of milliseconds since the previous line received.
When closed with CTRL+C, it prints a statistical summary of the time measured between receied lines.

It takes the following command line parameters, all of which are optional:

  -c N     After N lines are received, exit the program
  -q       Quiet mode. Do not print the timing of each line as it is received.
  -v       Verbose mode. Print the running statistics of the timing for each line.
  -g       Graph mode. Instead of timing information, print an ASCII bar graph.
  -n       No statistical summary. Suppress printing the statistical summary when exiting.
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
#include <ctype.h>
#include <getopt.h>
#include <time.h>

int lines_received = 0;
double total_time = 0.0;
double min_time = 0.0;
double max_time = 0.0;
double avg_time = 0.0;
int no_summary = 0;

void print_stat_summary()
{
  fprintf(stderr, "\nSummary:\n");
  fprintf(stderr, "  Total lines received: %d\n", lines_received);
  fprintf(stderr, "  Total time: %.3f ms\n", total_time);
  fprintf(stderr, "  Min time: %.3f ms\n", min_time);
  fprintf(stderr, "  Max time: %.3f ms\n", max_time);
  fprintf(stderr, "  Avg time: %.3f ms\n", avg_time);
}

void sig_handler(int _signum)
{
  // Handle SIGINT (CTRL+C) signal
  if( _signum == SIGINT )
  {
    // Print a message and exit
    fprintf(stderr, "\nReceived CTRL+C. Exiting...\n");
    if(!no_summary)
    {
      print_stat_summary();
    }
    fflush(stdout);
    exit(0);
  }
}

void print_usage()
{
  // Print usage information
  fprintf(stderr, "Usage: console_loopback_timer [-c N] [-q] [-v] [-g] [-n]\n");
  fprintf(stderr, "  -c N     After N lines are received, exit the program\n");
  fprintf(stderr, "  -q       Quiet mode. Do not print the timing of each line as it is received.\n");
  fprintf(stderr, "  -v       Verbose mode. Print the running statistics of the timing for each line.\n");
  fprintf(stderr, "  -g       Graph mode. Instead of timing information, print an ASCII bar graph.\n");
  fprintf(stderr, "  -n       No statistical summary. Suppress printing the statistical summary when exiting.\n");
}

int main(int argc, char *argv[])
{
  int c;
  int quiet = 0;
  int verbose = 0;
  int graph = 0;
  int count = -1; // -1 means no limit
  struct timeval start_time, end_time, elapsed_time;

  // Parse command line arguments
  while ((c = getopt(argc, argv, "c:qvgn")) != -1)
  {
    switch (c)
    {
    case 'c':
      count = atoi(optarg);
      break;
    case 'q':
      quiet = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'g':
      graph = 1;
      break;
    case 'n':
      no_summary = 1;
      break;
    default:
      print_usage();
      exit(EXIT_FAILURE);
    }
  }

  // Set a signal handler only for SIGINT (CTRL+C)
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  // Initialize start time
  gettimeofday(&start_time, NULL);

  // Main loop
  while (1)
  {
    char buffer[1024];
    ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0)
    {
      perror("read");
      exit(EXIT_FAILURE);
    }
    if (bytes_read == 0)
    {
      break; // EOF
    }

    buffer[bytes_read] = '\0'; // Null-terminate the string
    lines_received++;

    // Get current time
    gettimeofday(&end_time, NULL);

    // Calculate elapsed time
    timersub(&end_time, &start_time, &elapsed_time);
    double elapsed_ms = (elapsed_time.tv_sec + elapsed_time.tv_usec / 1e6) / 1e3;

    // Update total time and statistics
    total_time += elapsed_ms;
    if (lines_received == 1 || elapsed_ms < min_time)
    {
      min_time = elapsed_ms;
    }
    if (lines_received == 1 || elapsed_ms > max_time)
    {
      max_time = elapsed_ms;
    }
    avg_time = total_time / lines_received;

    // Print timing information
    if (!quiet)
    {
      if (graph)
      {
        int bar_length = (int)(elapsed_ms * 10); // Scale for graph
        printf("%s", buffer);
        for (int i = 0; i < bar_length; i++)
        {
          putchar('#');
        }
        putchar('\n');
      }
      else if (verbose)
      {
        printf("%s: %.3f ms (min: %.3f, max: %.3f, avg: %.3f)\n", buffer, elapsed_ms, min_time, max_time, avg_time);
      }
      else
      {
        printf("%s\n", buffer);
      }
    }

    // Check for count limit
    if (count > 0 && lines_received >= count)
    {
      break;
    }

    // Update start time for next iteration
    start_time = end_time;
  }
  // Print summary statistics
  if (!no_summary)
  {
    print_stat_summary();
  }
  return 0;
}
