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

ssize_t read_a_line(char *buffer, size_t size)
{
  // Read a line from stdin
  char *result = fgets(buffer, size, stdin);
  if (result) {
    ssize_t bytes_read = strlen(buffer);
    char *newline = strchr(buffer, '\n');
    if (newline) {
      *(newline + 1) = '\0'; // Truncate at newline
    }
    return bytes_read;
  } else if (ferror(stdin)) {
    perror("Error reading from stdin");
    return -1;
  } else {
    return 0; // EOF
  }
}

void print_stat_summary()
{
  fprintf(stderr, "\nSummary:\n");
  fprintf(stderr, "  Total lines received: %d\n", lines_received);
  fprintf(stderr, "  Total time: %.3f ms\n", total_time);
  fprintf(stderr, "  Min/Avg/Max time: %.3f/%.3f/%.3f ms\n", min_time, avg_time, max_time);
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
  fprintf(stderr, "Usage: console_loopback_timer [-c N] [-q] [-v] [-g] [-n] [-i N]\n");
  fprintf(stderr, "  -c N     After N lines are received, exit the program\n");
  fprintf(stderr, "  -q       Quiet mode. Do not print the received lines.\n");
  fprintf(stderr, "  -v       Verbose mode. Print the running statistics of the timing for each line.\n");
  fprintf(stderr, "  -g       Graph mode. Instead of timing information, print an ASCII bar graph.\n");
  fprintf(stderr, "  -n       No statistical summary. Suppress printing the statistical summary when exiting.\n");
  fprintf(stderr, "  -i N     Ignore the first N lines. This is useful to avoid measuring setting up the connection.\n");
}

int main(int argc, char *argv[])
{
  int c;
  int quiet = 0;
  int verbose = 0;
  int graph = 0;
  int count = -1; // -1 means no limit
  int lines_to_ignore = 0;
  struct timeval start_time, end_time, elapsed_time;

  // Parse command line arguments
  while ((c = getopt(argc, argv, "c:qvgni:")) != -1)
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
    case 'i':
      lines_to_ignore = atoi(optarg);
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
  
  if(lines_to_ignore > 0) {
    char buffer[1024];
    // Read and ignore the first set of lines
    for( int i = 0; i < lines_to_ignore; i++ )
    {
      ssize_t bytes_read = read_a_line(buffer, sizeof(buffer));
      if (bytes_read < 0)
      {
        perror("read");
        exit(EXIT_FAILURE);
      }
      if (bytes_read == 0)
      {
        fprintf(stderr, "EOF while ignoring input lines\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  // Main loop
  char buffer[1024];
  while (1)
  {
    ssize_t bytes_read = read_a_line(buffer, sizeof(buffer));
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
    double elapsed_ms = ((double)elapsed_time.tv_sec * 1000.0) + ((double)elapsed_time.tv_usec / 1000.0);

    // Update total time and statistics
    
    // ignore the first measurement b/c it will include start up time
    if( lines_received != 1) {
      total_time += elapsed_ms;
    }
      
    if (lines_received == 2 || elapsed_ms < min_time)
    {
      min_time = elapsed_ms;
    }
    if (lines_received == 2 || elapsed_ms > max_time)
    {
      max_time = elapsed_ms;
    }
    avg_time = total_time / lines_received;

    // Print stuff
    if(verbose) {
      printf("%.3f ms (min: %.3f, max: %.3f, avg: %.3f)\n", elapsed_ms, min_time, max_time, avg_time);
    }

    if(!quiet) {
      printf("%s\n", buffer);
    }

    if(graph) {
      int bar_length = (int)(elapsed_ms * 10); // Scale for graph
      for (int i = 0; i < bar_length; i++)
      {
        putchar('#');
      }
      putchar('\n');
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
