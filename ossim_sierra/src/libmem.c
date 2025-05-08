// #define MM_PAGING    //nhớ bật paging 

/*
 * System Library
 * Memory Module Library libmem.c 
 */

#include "string.h"
#include "mm.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

//static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER; //tạm bỏ tránh warning

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
    *alloc_addr = rgnode.rg_start;

    //pthread_mutex_unlock(&mmvm_lock); //fix bỏ
    return 0;
  }

  // nếu ko thấy vùng trống thì mở rộng segment (sbrk)
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);

  int old_sbrk = cur_vma->sbrk;//define thêm
  //printf("old_sbrk: %d\n", old_sbrk);

  if(!inc_vma_limit(caller, vmaid, inc_sz)){
    
    // update vm_freerg_list
    if(inc_sz > size){
      struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
      rgnode->rg_start = size + old_sbrk + 1; 
      rgnode->rg_end = inc_sz + old_sbrk;
      enlist_vm_freerg_list(caller->mm, rgnode);
    }
    cur_vma->sbrk += inc_sz;
    // printf("########## sbrk: %ld\n", cur_vma->sbrk);
    /*Successful increase limit */
    caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
    caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
    *alloc_addr = old_sbrk;
    //printf("alloc_addr: %d\n", *alloc_addr);
  }

  return 0;

}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  //thu hồi bộ nhớ ảo theo rgid được cấp phát
  //không giải phóng thật sự bộ nhớ vật lý, chỉ đưa vùng đã dùng vào freerg_list

  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return -1;

  // struct vm_rg_struct rgnode;

  // // Lấy vùng đã cấp phát từ bảng symbol table
  // rgnode.rg_start = caller->mm->symrgtbl[rgid].rg_start;
  // rgnode.rg_end = caller->mm->symrgtbl[rgid].rg_end;
  // //rgnode.vmaid = vmaid;

  // // Nếu chưa cấp phát thì không làm gì cả
  // if (rgnode.rg_start == 0 && rgnode.rg_end == 0)
  //   return -1;
  
  /* Thêm vùng này vào danh sách vùng trống */
  enlist_vm_freerg_list(caller->mm, &caller->mm->symrgtbl[rgid]);

  /* Reset symbol table entry để tránh bị dùng lại */
  caller->mm->symrgtbl[rgid].rg_start = 0;
  caller->mm->symrgtbl[rgid].rg_end = 0;

  return 0;
}

/*liballoc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
    // Kiểm tra xem reg_index có hợp lệ không
    if (reg_index >= PAGING_MAX_SYMTBL_SZ) {
        return -1;
    }

    int allocated_addr = 0;
    // Gọi hàm __alloc để thực hiện cấp phát thực sự
    int ret = __alloc(proc, 0, reg_index, size, &allocated_addr);
    if (ret != 0) {
        // Nếu cấp phát thất bại
        printf("Allocation failed for PID=%d region=%d size=%u\n",
               proc->pid, reg_index, size);
        return -1;
    }

#ifdef IODUMP
    // In thông tin nếu cần
    printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
    printf("PID=%d - Region=%d - Address=%08x - Size=%u byte\n",
           proc->pid, reg_index, allocated_addr, size);
#ifdef PAGETBL_DUMP
    print_pgtbl(proc, 0, -1);
#endif
    //MEMPHY_dump(proc->mram);
    printf("================================================================\n");
#endif

    return 0;
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
     // Kiểm tra xem reg_index có hợp lệ không
     if (reg_index >= PAGING_MAX_SYMTBL_SZ) {
         return -1;
     }
 
     // Gọi hàm __free để thực hiện giải phóng
     int ret = __free(proc, 0, reg_index);
     if (ret != 0) {
         // Nếu giải phóng thất bại
         printf("Freeing region %d failed for PID=%d\n", reg_index, proc->pid);
         return -1;
     }
 
 #ifdef IODUMP
     // In thông tin nếu cần
     printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
     printf("PID=%d - Region=%d\n", proc->pid, reg_index);
 #ifdef PAGETBL_DUMP
    print_pgtbl(proc, 0, -1);
 #endif
     printf("================================================================\n");
 #endif
 
     return 0;
 }

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];
  if(pte < 0)
    return -1; // Không có trang nào trong RAM

  if (!PAGING_PAGE_PRESENT(pte))
  { 
    /* Page is not in RAM, need to bring it in */
    int vicpgn, vicfpn, swpfpn;
    uint32_t vicpte;
    int tgtfpn = GETVAL(pte, PAGING_PTE_SWPOFF_MASK,PAGING_PTE_SWPOFF_LOBIT);//the target frame storing our variable

    // 1. tìm victim page đang ở trong RAM
    find_victim_page(caller->mm, &vicpgn);
    vicpte = mm->pgd[vicpgn];
    vicfpn = GETVAL(vicpte, PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT);

    // 2. lấy 1 frame trong vùng SWAP để chứa victim
    MEMPHY_get_freefp(caller->active_mswp, &swpfpn);

    // 3. swap victim từ RAM → SWAP
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);

    // 4. swap trang cần dùng từ SWAP → RAM (gán vào frame của victim)
    __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
    MEMPHY_put_freefp(caller->active_mswp, tgtfpn); 

    // 5. Cập nhật page table
    pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);     // Ghi nhận victim đã bị swap ra, truyền 0 vì có 1 loại swap 
    pte_set_fpn(&mm->pgd[pgn], tgtfpn);         // Ghi nhận page mới vào frame RAM

    // 6. Thêm page mới vào FIFO
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
    
    // 7. Nếu đã có trong RAM, trả về frame number
    *fpn = PAGING_FPN(mm->pgd[pgn]);
    return 0;
  }
  *fpn = GETVAL(pte, PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT);
  //printf("pg_getpage: %d\n", *fpn);
  return 0;
}


/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  //đọc gtri tại địa chỉ addr trong vùng nhớ ảo
  //nếu addr không nằm trong vùng nhớ ảo thì trả về lỗi
  // dung syscall với SYSMEM_IO_READ để đọc dữ liệu từ physical memory
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Đảm bảo page đang ở RAM (swap nếu cần)
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; // Truy cập page lỗi

  // Tính địa chỉ vật lý
  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off; // << là dịch trái
  //printf("pg_getval: %d\n", phyaddr);
  MEMPHY_read(caller->mram, phyaddr, data);

  return 0;
}


/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  //ghi gtri tại địa chỉ addr trong vùng nhớ ảo
  // nếu page chứa địa chỉ đó chưa có trong RAM thì swap vào
  // tính địa chỉ vật lý và dùng syscall để ghi gtri
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Đảm bảo page đang nằm trong RAM (swap nếu cần)
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; // Truy cập lỗi

  // Tính địa chỉ vật lý từ fpn và offset
  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off; // << là dịch trái
  //printf("Inside pg_setval, fpn: %d, phyaddr: %u\n", fpn, phyaddr);

  MEMPHY_write(caller->mram,phyaddr, value);
  //printf("The value of RAM at address %d is: %d", phyaddr, caller->mram->storage[phyaddr]);

  return 0;
}


/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  //printf("=== READ CALLED ===\n"); //debug
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
    return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  return 0;
}

/*libread - PAGING-based read a region memory */
int libread(
  //gọi __read từ source cộng thêm offset, trả về giá trị đọc được
  // __read sẽ gọi pg_getval để swap vào RAM nếu cần
  // sau khi có data kiểu byte thì chuyển byte về kiểu uint32_t
  // và gán vào destination
  struct pcb_t *proc,      // Process executing the instruction
  uint32_t source,         // Index of source register
  uint32_t offset,         // Source address = [source] + [offset]
  uint32_t* destination)   // Nơi lưu kết quả đọc được
{
//printf("=== LIBREAD CALLED ===\n"); //debug
BYTE data;
int val = __read(proc, 0, source, offset, &data);

// Gán dữ liệu đọc được vào destination
*destination = (uint32_t)data;


#ifdef IODUMP
  printf("===== PHYSICAL MEMORY AFTER READING =====\n");
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // In bảng trang nếu cần
#endif
  MEMPHY_dump(proc->mram);  // In bộ nhớ vật lý
#endif

return val; // 0 nếu OK, -1 nếu lỗi
}


/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  //printf("=== WRITE CALLED ===\n"); //debug
  
  //get_symrg_byid để tìm vùng nhớ rgid
  // get_vma_by_num để tìm vma tương ứng
  //gọi pg_setval để ghi giá trị vào địa chỉ trong vùng nhớ
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
    return -1;
  
  //printf("Rgid: %d\n", rgid);

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  return 0;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
  //gọi __write để thực hiện ghi như trên 
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    uint32_t offset)
{
//printf("=== LIBWRITE CALLED ===\n"); //debug

#ifdef IODUMP
  printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return __write(proc, 0, destination, offset, data);
}



/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  //thu hồi tất cả các frame (memram và memswap) trong vùng nhớ của caller
  //duyệt pagetable
  // nếu trag ko present nằm ở memswap lấy frame số từ paging pte swp trả lại active mswp
  // nếu trang present lấy fpn bằng paging pte fpn trả lại mram
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
  //Chọn ra trang đang dùng trong bộ nhớ RAM để thay thế (swap out) khi cần thêm chỗ chứa page mới
  
  struct pgn_t *pg = mm->fifo_pgn;

  if (pg == NULL)
    return -1; // Không có trang nào để chọn làm nạn nhân
  if (pg->pg_next == NULL){
    // Không có trang nào để chọn làm nạn nhân
    *retpgn = pg->pgn; // Lấy số trang nạn nhân đầu tiên trong FIFO

    mm->fifo_pgn = pg->pg_next; // Gỡ node đầu tiên ra khỏi danh sách FIFO

    free(pg); // Giải phóng node đã lấy
  }

  struct pgn_t *prev_pg = mm ->fifo_pgn;
  while(pg->pg_next) {
    prev_pg = pg;
    pg = pg -> pg_next;
  }
  *retpgn = pg->pgn;
  prev_pg->pg_next = NULL; 
  free(pg);
  return 0;   
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  //tìm một vùng nhớ ảo trống đủ lớn để cấp phát cho process.
  //Duyệt từng node trong danh sách rgit, nếu vùng nào có độ dài >= size thì:
  //gán newrg->rg_start = rgit->rg_start
  //gán newrg->rg_end = rg_start + size
  //cập nhật lại rgit->rg_start += size (phần còn lại nếu có)
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1; // Không có vùng trống nào để cấp phát

  newrg->rg_start = newrg->rg_end = -1; // Khởi tạo giá trị mặc định

  while (rgit != NULL) {
    int len = rgit->rg_end - rgit->rg_start;

    if (len >= size) {
      // Tìm thấy vùng trống đủ lớn
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      // Cập nhật lại vùng trống sau khi cấp phát
      rgit->rg_start += size;

      // Nếu vùng trống này đã cấp phát hết thì loại khỏi danh sách
      if (rgit->rg_start == rgit->rg_end) {
        // Gỡ node này ra khỏi danh sách
        struct vm_rg_struct *prev = cur_vma->vm_freerg_list;

        if (prev == rgit) {
          cur_vma->vm_freerg_list = rgit->rg_next;
        } else {
          while (prev->rg_next != rgit)
            prev = prev->rg_next;

          prev->rg_next = rgit->rg_next;
        }

        free(rgit);
      }

      return 0;
    }

    rgit = rgit->rg_next;
  }

  return -1; // Không tìm thấy vùng phù hợp
}


//#endif
