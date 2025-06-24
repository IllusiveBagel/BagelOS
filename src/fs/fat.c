
#include "sd.h"
#include "uart.h"

// get the end of bss segment from linker
extern unsigned char _end;

static unsigned int partitionlba = 0;

// the BIOS Parameter Block (in Volume Boot Record)
typedef struct
{
    char jmp[3];
    char oem[8];
    unsigned char bps0;
    unsigned char bps1;
    unsigned char spc;
    unsigned short rsc;
    unsigned char nf;
    unsigned char nr0;
    unsigned char nr1;
    unsigned short ts16;
    unsigned char media;
    unsigned short spf16;
    unsigned short spt;
    unsigned short nh;
    unsigned int hs;
    unsigned int ts32;
    unsigned int spf32;
    unsigned int flg;
    unsigned int rc;
    char vol[6];
    char fst[8];
    char dmy[20];
    char fst2[8];
} __attribute__((packed)) bpb_t;

// directory entry structure
typedef struct
{
    char name[8];
    char ext[3];
    char attr[9];
    unsigned short ch;
    unsigned int attr2;
    unsigned short cl;
    unsigned int size;
} __attribute__((packed)) fatdir_t;

// Static Buffer for reading MBR and boot record
static unsigned char fat_buf[1024]; // 1KB for safety, or at least 512 bytes

/**
 * Get the starting LBA address of the first partition
 * so that we know where our FAT file system starts, and
 * read that volume's BIOS Parameter Block
 */
int fat_getpartition(void)
{
    unsigned char *mbr = fat_buf;
    bpb_t *bpb = (bpb_t *)fat_buf;
    uart_puts("Reading MBR...\n");
    // read the partitioning table
    if (sd_readblock(0, fat_buf, 1))
    {
        uart_puts("MBR read OK\n");
        // check magic
        uart_puts("Checking magic...\n");
        if (mbr[510] != 0x55 || mbr[511] != 0xAA)
        {
            uart_puts("ERROR: Bad magic in MBR\n");
            return 0;
        }
        // check partition type
        uart_puts("Checking partition type...\n");
        if (mbr[0x1C2] != 0xE /*FAT16 LBA*/ && mbr[0x1C2] != 0xC /*FAT32 LBA*/)
        {
            uart_puts("ERROR: Wrong partition type\n");
            return 0;
        }
        uart_puts("MBR disk identifier: ");
        uart_hex(*((unsigned int *)((unsigned long)fat_buf + 0x1B8)));
        uart_puts("\nFAT partition starts at: ");
        uart_puts("\nParsing partition LBA...\n");
        uart_puts("Partition LBA bytes: ");
        uart_hex(mbr[0x1C9]);
        uart_puts(" ");
        uart_hex(mbr[0x1C8]);
        uart_puts(" ");
        uart_hex(mbr[0x1C7]);
        uart_puts(" ");
        uart_hex(mbr[0x1C6]);
        uart_puts("\n");
        // should be this, but compiler generates bad code...
        // partitionlba = *((unsigned int *)((unsigned long)fat_buf + 0x1C6));
        partitionlba = mbr[0x1C6] + (mbr[0x1C7] << 8) + (mbr[0x1C8] << 16) + (mbr[0x1C9] << 24);
        uart_puts("Partition LBA: ");
        uart_hex(partitionlba);
        uart_puts("\n");
        // read the boot record
        uart_puts("\nReading boot record...\n");
        if (!sd_readblock(partitionlba, fat_buf, 1))
        {
            uart_puts("ERROR: Unable to read boot record\n");
            return 0;
        }
        uart_puts("Boot record read OK\n");
        // check file system type. We don't use cluster numbers for that, but magic bytes
        if (!(bpb->fst[0] == 'F' && bpb->fst[1] == 'A' && bpb->fst[2] == 'T') &&
            !(bpb->fst2[0] == 'F' && bpb->fst2[1] == 'A' && bpb->fst2[2] == 'T'))
        {
            uart_puts("ERROR: Unknown file system type\n");
            return 0;
        }
        uart_puts("FAT type: ");
        // if 16 bit sector per fat is zero, then it's a FAT32
        uart_puts(bpb->spf16 > 0 ? "FAT16" : "FAT32");
        uart_puts("\n");
        return 1;
    }
    else
    {
        uart_puts("ERROR: Unable to read MBR\n");
        return 0;
    }
    return 0;
}

/**
 * List root directory entries in a FAT file system
 */
void fat_listdirectory(void)
{
    bpb_t *bpb = (bpb_t *)fat_buf;
    fatdir_t *dir = (fatdir_t *)fat_buf;
    unsigned int root_sec, s;
    // find the root directory's LBA
    root_sec = ((bpb->spf16 ? bpb->spf16 : bpb->spf32) * bpb->nf) + bpb->rsc;
    s = (bpb->nr0 + (bpb->nr1 << 8));
    uart_puts("FAT number of root diretory entries: ");
    uart_hex(s);
    s *= sizeof(fatdir_t);
    if (bpb->spf16 == 0)
    {
        // adjust for FAT32
        root_sec += (bpb->rc - 2) * bpb->spc;
    }
    // add partition LBA
    root_sec += partitionlba;
    uart_puts("\nFAT root directory LBA: ");
    uart_hex(root_sec);
    uart_puts("\n");
    // load the root directory
    if (sd_readblock(root_sec, (unsigned char *)fat_buf, s / 512 + 1))
    {
        uart_puts("\nAttrib Cluster  Size     Name\n");
        // iterate on each entry and print out
        for (; dir->name[0] != 0; dir++)
        {
            // is it a valid entry?
            if (dir->name[0] == 0xE5 || dir->attr[0] == 0xF)
                continue;
            // decode attributes
            uart_send(dir->attr[0] & 1 ? 'R' : '.');  // read-only
            uart_send(dir->attr[0] & 2 ? 'H' : '.');  // hidden
            uart_send(dir->attr[0] & 4 ? 'S' : '.');  // system
            uart_send(dir->attr[0] & 8 ? 'L' : '.');  // volume label
            uart_send(dir->attr[0] & 16 ? 'D' : '.'); // directory
            uart_send(dir->attr[0] & 32 ? 'A' : '.'); // archive
            uart_send(' ');
            // staring cluster
            uart_hex(((unsigned int)dir->ch) << 16 | dir->cl);
            uart_send(' ');
            // size
            uart_hex(dir->size);
            uart_send(' ');
            // filename
            dir->attr[0] = 0;
            uart_puts(dir->name);
            uart_puts("\n");
        }
    }
    else
    {
        uart_puts("ERROR: Unable to load root directory\n");
    }
}