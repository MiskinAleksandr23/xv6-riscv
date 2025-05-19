#include <endian.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define SUPER_SIZE 1024

uint64_t file_size;
FILE* file_system;
uint32_t block_size;

void cleanup_failure(FILE* fs, uint8_t* buffer) {
  if (fs) fclose(fs);
  if (buffer) free(buffer);
  exit(EXIT_FAILURE);
}

void direct_block(uint32_t block_pointer) {
  if (file_size <= 0) return;

  size_t output_bytes = file_size < block_size ? file_size : block_size;

  uint8_t* block_data = malloc(output_bytes);

  if (!block_data) {
    perror("malloc failed");
    cleanup_failure(file_system, block_data);
  }

  if (block_pointer == 0) {
    memset(block_data, 0, output_bytes);
  } else {
    if (fseek(file_system, (long) block_pointer * block_size, SEEK_SET) != 0) {
      perror("fseek failed");
      cleanup_failure(file_system, block_data);
    }
    if (fread(block_data, output_bytes, 1, file_system) != 1) {
      perror("fread failed");
      cleanup_failure(file_system, block_data);
    }
  }
  if (fwrite(block_data, 1, output_bytes, stdout) != output_bytes) {
    perror("fwrite failed");
    cleanup_failure(file_system, block_data);
  }
  file_size -= output_bytes;
  free(block_data);
}
void indirect_block(uint32_t block_pointer, int level) {
  if (!block_pointer) {
    uint8_t* zeroes = calloc(block_size, 1);
    if (!zeroes) {
      perror("calloc failed");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < (int) block_size / 4; ++i) {
      size_t output_bytes = file_size < block_size ? file_size : block_size;
      fwrite(zeroes, output_bytes, 1, stdout);
      file_size -= output_bytes;
      if (file_size <= 0) return;
    }
    free(zeroes);
    return;
  }
  uint32_t* blocks = malloc(block_size);
  if (!blocks) {
    perror("malloc failed");
    exit(EXIT_FAILURE);
  }

  if (fseek(file_system, (int64_t)block_pointer * block_size, SEEK_SET)) {
    perror("fseek failed");
    free(blocks);
    exit(EXIT_FAILURE);
  }

  if (fread(blocks, block_size, 1, file_system) != 1) {
    perror("fread failed");
    free(blocks);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < (int)block_size / 4 && file_size > 0; ++i) {
    uint32_t block = blocks[i];
    if (block == 0) continue;
    if (level > 0) {
      indirect_block(block, level - 1);
    } else {
      direct_block(block);
    }
  }
  free(blocks);
};

int main(int argc, char** argv) {
  if (argc != 3) {
    perror("format must be: <filesystem> <inode>\n");
    exit(EXIT_FAILURE);
  }
  int inode = atoi(argv[2]);
  file_system = fopen(argv[1], "rb");
  if (!file_system) {
    perror("fopen error");
    exit(EXIT_FAILURE);
  }

  uint32_t log_block_size, blocks_per_group, inode_per_group, inode_size;
  if (fseek(file_system, SUPER_SIZE + 24, SEEK_SET) != 0) {
    perror("fseek failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  if (fread(&log_block_size, 4, 1, file_system) != 1) {
    perror("fread failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  log_block_size = le32toh(log_block_size);
  block_size = 1024 << log_block_size;
  uint32_t descriptor_table = (block_size == 1024) ? 2 : 1;

  if (fseek(file_system, SUPER_SIZE + 32, SEEK_SET) != 0) {
    perror("fseek failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  if (fread(&blocks_per_group, 4, 1, file_system) != 1) {
    perror("fread failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  blocks_per_group = le32toh(blocks_per_group);

  if (fseek(file_system, SUPER_SIZE + 40, SEEK_SET) != 0) {
    perror("fseek failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  if (fread(&inode_per_group, 4, 1, file_system) != 1) {
    perror("fread failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  inode_per_group = le32toh(inode_per_group);

  if (fseek(file_system, SUPER_SIZE + 88, SEEK_SET) != 0) {
    perror("fseek failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  if (fread(&inode_size, 4, 1, file_system) != 1) {
    perror("fread failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  inode_size = le32toh(inode_size);

  uint32_t inode_group = (inode - 1) / inode_per_group;

  if (fseek(file_system, descriptor_table * block_size + inode_group * 32, SEEK_SET) != 0) {
    perror("fseek failed");
    fclose(file_system);
    exit(1);
  }

  uint32_t addr_inode_table;
  if (fseek(file_system, 8, SEEK_CUR) != 0) {
    perror("fseek failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  if (fread(&addr_inode_table, 4, 1, file_system) != 1) {
    perror("fread failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  addr_inode_table = le32toh(addr_inode_table);

  uint32_t inode_index_in_group = (inode - 1) % inode_per_group;
  uint32_t inode_offset = addr_inode_table * block_size + inode_index_in_group * inode_size;

  if (fseek(file_system, inode_offset, SEEK_SET) != 0) {
    perror("fseek to inode failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }

  uint32_t lower_bits;
  if (fseek(file_system, 4, SEEK_CUR) != 0) {
    perror("fseek to inode failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  if (fread(&lower_bits, 4, 1, file_system) != 1) {
    perror("fread failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }

  uint32_t upper_bits;
  if (fseek(file_system, 104, SEEK_CUR) != 0) {
    perror("fseek to inode failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  if (fread(&upper_bits, 4, 1, file_system) != 1) {
    perror("fread failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }
  lower_bits = le32toh(lower_bits);
  upper_bits = le32toh(upper_bits);

  file_size = lower_bits + (((long long)upper_bits) << 32);

  if (fseek(file_system, inode_offset, SEEK_SET) != 0) {
    perror("fseek to inode failed");
    fclose(file_system);
    exit(EXIT_FAILURE);
  }

  uint32_t block_pointers[15];
  for (int i = 0; i < 15; i++) {
    if (fseek(file_system, 40, SEEK_CUR) != 0) {
      perror("fseek to inode failed");
      fclose(file_system);
      exit(EXIT_FAILURE);
    }
    if (fread(&block_pointers[i], 4, 1, file_system) != 1) {
      perror("fread failed");
      fclose(file_system);
      exit(EXIT_FAILURE);
    }
    block_pointers[i] = le32toh(block_pointers[i]);
  }

  for (int i = 0; i < 15; ++i) {
    if (i < 12)
      direct_block(block_pointers[i]);
    else
      indirect_block(block_pointers[i], i - 11);
  }
  fclose(file_system);
  return 0;
}
