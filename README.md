# Overview
Determine the pattern of data corruption or loss!  
It writes 8-byte segments to the test file. Each has a 4-byte magic value and another 4 bytes to indicate the offset of the segment. This enables the detection of corrupt segments and malalignment (e.g. in case of data loss).

# Usage
* Create new test file of sufficient size:  
` $ truncate -s [SIZE] [FILE]`  
` $ cpatt w [FILE]`

* After manipulation, test for corruption:  
` $ cpatt t [FILE]`
