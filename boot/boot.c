#include "boot.h"

// DO NOT DEFINE ANY NON-LOCAL VARIBLE!

void load_kernel() {
  //  char hello[] = {'\n', 'h', 'e', 'l', 'l', 'o', '\n', 0};
  //  putstr(hello);
  //  while (1) ;
  // remove both lines above before write codes below
  Elf32_Ehdr *elf = (void *)0x8000;  //elf文件
  copy_from_disk(elf, 255 * SECTSIZE, SECTSIZE); //sectsize是扇区大小
  Elf32_Phdr *ph, *eph;
  ph = (void*)((uint32_t)elf + elf->e_phoff); //ph 是程序头表的位置
  eph = ph + elf->e_phnum;  //e_phnum 是表项的个数
  for (; ph < eph; ph++) {
    if (ph->p_type == PT_LOAD) {
      // TODO: Lab1-2, Load kernel and jump
      // 将ELF文件中起始于p_offset，大小为p_filesz字节的数据拷贝到内存中起始于p_vaddr的位置，并将内存中剩余的p_memsz - p_filesz字节的内容清零。
        uint32_t src = (uint32_t)elf + ph->p_offset;
        uint32_t dest = ph->p_vaddr;
        uint32_t filesz = ph->p_filesz;
        uint32_t memsz = ph->p_memsz;

        // 使用 memcpy 进行数据拷贝
        memcpy((void *)dest, (const void *)src, filesz);

        // 使用 memset 清零剩余空间
        memset((void *)(dest + filesz), 0, memsz - filesz);

    }
  }
  uint32_t entry = elf->e_entry; // change me
  ((void(*)())entry)();
}
