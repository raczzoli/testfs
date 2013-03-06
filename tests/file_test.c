#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main()
{

	FILE *fp 			= fopen("/mnt/test_file.txt", "w+");
	int num_bytes 			= 0;
	int ret				= 0;
	char source_buffer[]		= "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam ac erat justo, nec lobortis metus. Nullam interdum varius commod";
	char target_buffer[1024];

	memset(target_buffer, 0x00, sizeof target_buffer);

	if (fp == NULL)
		printf("fopen failed.\n");
	else
		printf("fopen succeeded.\n");

	if (fp) {

		ret = fseek(fp, 0, SEEK_SET);

		if (ret) {
			printf("fseek failed: %s\n", strerror(errno));
		}

		num_bytes = fwrite(source_buffer, 1, sizeof source_buffer, fp);
		printf("bytes written: %d.\n", num_bytes);

		ret = fseek(fp, 0, SEEK_SET);
		if (ret) {
                        printf("fseek failed: %s\n", strerror(errno));
                }

		num_bytes = fread(target_buffer, 1, sizeof source_buffer, fp);
		printf("bytes read: %d.\n", num_bytes);		

		printf("----------------------------------\n%s\n----------------------------------------------\n", target_buffer);

		fclose(fp);
	}

	return 0;
}

