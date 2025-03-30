# jspon
A lightweight JSON parser wrtten in C.

## DISCLAIMER
This is a personal project, the security of this project has not been audited and therefore should not be used to parse untrusted data. I have done my best to prevent any this, however I cannot ensure 100% reliability. If you are looking for a secure, fast, and well tested library, I would recommend [cJSON](https://github.com/DaveGamble/cJSON).

## Usage
jspon is a very simple library to use, only containing three functions.

- `jspon_get_values`
- `jspon_get_array_size`
- `jspon_parse_array`

#### ```int jspon_get_values(char* json, size_t path_num, char** paths, char** bufs, size_t* buf_sizes)```
#### Return values:
- `0`:  No errors.
- `-1`: One or more values were not found, in this case all found values are still written to their buffers.
- `-2`: Specified path was too large, in this case no values are written. 

This function takes a JSON string, a list of paths and a list of buffers. The value found at each path is written to the corresponding buffer. Paths should be in the format `"item.subitem.value"`. All values are treated as strings when being written to the buffer.

### Example

**main.c:**
```c
int main()
{
    char content[65536];
	FILE* f = fopen("example.json","r");
	fread(content, sizeof(char), 65536, f);
	fclose(f);

	const int LEN = 3;
	
	char* bufs[LEN];
	size_t buf_sizes[LEN];
	char* paths[LEN];

	bufs[0] = malloc(32); // Malloc must align with buf_sizes
	bufs[1] = malloc(64);
	bufs[2] = malloc(128);
	buf_sizes[0] = 32;
	buf_sizes[1] = 64;
	buf_sizes[2] = 128;
	paths[0] = "val1";
	paths[1] = "nested.2";
	paths[2] = "nested";

	int ret_val = jspon_get_values(content, LEN, paths, bufs, buf_sizes);
	printf("%s\n%s\n%s", bufs[0], bufs[1], bufs[2]);
	return ret_val;
}
```

**example.json:**
```json
{
    "val1": 123,
    "nested": {
        "1": "one",
        "2": "two",
        "3": "three"
    }
}
```

**output:**
```
123
two
{1:"one",2:"two",3:"three"}
```
---
 #### ```size_t jspon_get_array_size(char* json)```
 
 This function takes a string of a JSON array, and returns the number of elements in the array.
### Example

**main.c:**
```c
int main()
{
	char* json = "[0,1,2,{'val1':3,'val2':4},'string']";
	printf("%zu\n", jspon_get_array_size(json));
	return 0;
}
```

**output:**
```
5
```
---
#### ```int jspon_parse_array(char* json, size_t arr_size, size_t buf_size, char** bufs)```
#### Return values:
- `0`:  No errors.
- `-1`: Number of elements in array surpassed `arr_size`, in this case, all buffers up to this point are written to.

This function takes a string of JSON, and a list of buffers, and writes each item in the array to each of the buffers.
### Example

**main.c:**
```c
int main()
{
	char* json = "[0,1,2,{'val1':3,'val2':4},'string']";
	size_t arr_size =  jspon_get_array_size(json);
	char** bufs = malloc(arr_size * sizeof(char*));
	const size_t buf_size = 128;
	for (size_t i=0; i<arr_size; ++i) {
		bufs[i] = malloc(buf_size);
	}

	int ret_val = jspon_parse_array(json, arr_size, buf_size, bufs);

	for (size_t i=0; i<arr_size; ++i) {
		printf("%s\n",bufs[i]);
	}

	return ret_val;
}
```

**output:**
```
0
1
2
{'val1':3,'val2':4}
string
```
## Building
I have provided no build tools for jspon since it is a very simple library. It has no dependencies and can easily be statically compiled with any C compiler alongside your code.  
```
gcc jspon.c main.c -o main
```


You could also compile it dynamically.  
```
gcc -shared jspon.c libjspon.so
```


These build examples are designed for POSIX based systems.
