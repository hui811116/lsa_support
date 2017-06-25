# lsa_support
Some small programs to support the experimental measurements of gr-lsa

Usage:
  data_reader {data file} {result file}

  data file: should be bytes in hex and store in .csv format
  
  result file: use "message_file_sink" in gr-lsa. 
  
  Information contain: 
    1) time, 
    2) bytes, 
    3) packets,
    4) error bits
  
