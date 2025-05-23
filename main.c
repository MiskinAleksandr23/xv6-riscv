#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

uint64_t file_size;
FILE* file_system;
uint32_t block_size;

#define SUPER_SIZE 1024
#define EXT2_BLOCK_SIZE 1024
#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SUPER_MAGIC 0xEF53

typedef struct {
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_r_blocks_count;
  uint32_t s_free_blocks_count;
  uint32_t s_free_inodes_count;
  uint32_t s_first_data_block;
  uint32_t s_log_block_size;
  uint32_t s_log_frag_size;
  uint32_t s_blocks_per_group;
  uint32_t s_frags_per_group;
  uint32_t s_inodes_per_group;
  uint32_t s_mtime;
  uint32_t s_wtime;
  uint16_t s_mnt_count;
  uint16_t s_max_mnt_count;
  uint16_t s_magic;
  char s_padding_2[18];
  uint32_t s_rev_level;
  char s_padding_1[4];
  uint32_t s_first_ino;
  uint16_t s_inode_size;
  uint16_t s_block_group_nr;
  uint32_t s_feature_compat;
  uint32_t s_feature_incompat;
  uint32_t s_feature_ro_compat;
  char s_padding[920];
} __attribute__((packed)) ext2_superblock_t;

typedef struct {
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_bitmap;
  uint32_t bg_inode_table;
  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs_count;
  char bg_padding[14];
} __attribute__((packed)) ext2_bg_desc_t;

typedef struct {
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size;
  uint32_t i_time[4];
  uint16_t i_gid;
  uint16_t i_links_count;
  uint32_t i_blocks;
  uint32_t i_flags;
  uint32_t i_osd1;
  uint32_t i_block[15];
  uint32_t i_gen;
  uint32_t i_file_acl;
  uint32_t i_dir_acl;
  uint32_t i_faddr;
  char i_osd2[12];
} __attribute__((packed)) ext2_inode_t;

void cleanup_failure(FILE* fs, uint8_t* buffer) {
  if (fs) fclose(fs);
  if (buffer) free(buffer);
  exit(EXIT_FAILURE);
}

static int transfer_block_data(FILE *fs_image, uint32_t block_num,
                            uint32_t block_size, uint64_t *remaining_bytes) {
  if (block_num == 0 || *remaining_bytes == 0) {
    return 0;
  }

  const off_t offset = (off_t)block_num * block_size;
  if (fseeko(fs_image, offset, SEEK_SET) != 0) {
    perror("Filesystem seek error");
    fclose(fs_image);
    return 1;
  }

  const size_t transfer_size = (*remaining_bytes > block_size)
                              ? block_size
                              : (size_t)*remaining_bytes;
  uint8_t data_buffer[block_size];

  if (fread(data_buffer, 1, transfer_size, fs_image) != transfer_size) {
    perror("Data read failure");
    return -1;
  }

  if (fwrite(data_buffer, 1, transfer_size, stdout) != transfer_size) {
    perror("Data write failure");
    return -1;
  }
  *remaining_bytes -= transfer_size;
  return 0;
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

int write_zeros(uint32_t block_size, uint64_t* bytes_remaining) {
  uint32_t to_write = *bytes_remaining > block_size ? block_size : *bytes_remaining;
  char zero_block[block_size];
  memset(zero_block, 0, block_size);

  if (fwrite(zero_block, 1, to_write, stdout) < to_write) {
      perror("writing block");
      return -1;
  }
  *bytes_remaining -= to_write;
  return 0;
}

static int process_indirect(FILE *file, uint32_t block_ptr, int level, uint32_t blk_size, uint64_t *remaining) {
    if (!*remaining) return 0;

    uint32_t entries = blk_size / 4;
    uint32_t total_blocks = entries;

    if (!block_ptr) {
        for (int i = 1; i < level; i++) total_blocks *= entries;
        for (uint32_t i = 0; i < total_blocks && *remaining; i++) {
            if (write_zeros(blk_size, remaining) < 0) return -1;
        }
        return 0;
    }

    uint32_t block_table[entries];

    if (fseek(file, block_ptr * blk_size, SEEK_SET)) {
        perror("seek failed");
        fclose(file);
        return -1;
    }

    if (fread(block_table, 4, entries, file) != entries) {
        perror("read failed");
        return -1;
    }

    for (uint32_t i = 0; i < entries && *remaining; i++) {
        if (level == 1) {
            if (block_table[i]) {
                if (transfer_block_data(file, block_table[i], blk_size, remaining) < 0) return -1;
            } else {
                if (write_zeros(blk_size, remaining) < 0) return -1;
            }
        } else {
            if (process_indirect(file, block_table[i], level - 1, blk_size, remaining) < 0) return -1;
        }
    }

    return 0;
}

int read_inode_data(FILE* f, uint64_t size, ext2_inode_t inode, uint32_t block_size) {
  uint64_t rem = size;

  for (int i = 0; i < 12 && rem > 0; i++) {
    if (transfer_block_data(f, inode.i_block[i], block_size, &rem) < 0)
      return -1;
  }

  if (process_indirect(f, inode.i_block[12], 1, block_size, &rem) < 0)
    return -1;
  if (process_indirect(f, inode.i_block[13], 2, block_size, &rem) < 0)
    return -1;
  if (process_indirect(f, inode.i_block[14], 3, block_size, &rem) < 0)
    return -1;
  return 0;
}

void convert_sb(ext2_superblock_t *sb) {
  sb->s_inodes_count = le32toh(sb->s_inodes_count);
  sb->s_blocks_count = le32toh(sb->s_blocks_count);
  sb->s_first_data_block = le32toh(sb->s_first_data_block);
  sb->s_blocks_per_group = le32toh(sb->s_blocks_per_group);
  sb->s_inodes_per_group = le32toh(sb->s_inodes_per_group);
  sb->s_log_block_size = le32toh(sb->s_log_block_size);
  sb->s_magic = le16toh(sb->s_magic);
  sb->s_inode_size = le16toh(sb->s_inode_size);
  sb->s_rev_level = le32toh(sb->s_rev_level);
}

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

  if (fseek(file_system, EXT2_SUPERBLOCK_OFFSET, SEEK_SET) != 0) {
      perror("error: fseek superblock");
      fclose(file_system);
      return 1;
  }
  ext2_superblock_t sb;
  if (fread(&sb, sizeof(ext2_superblock_t), 1, file_system) != 1) {
      perror("error: fread superblock");
      fclose(file_system);
      return 1;
  }

  convert_sb(&sb);

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
