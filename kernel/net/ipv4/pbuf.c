/**
 * @author Ali Mirmohammad
 * @file pbuf.c
 ** This file is part of AliNix.

**AliNix is free software: you can redistribute it and/or modify
**it under the terms of the GNU Affero General Public License as published by
**the Free Software Foundation, either version 3 of the License, or
**(at your option) any later version.

**AliNix is distributed in the hope that it will be useful,
**but WITHOUT ANY WARRANTY; without even the implied warranty of
**MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**GNU Affero General Public License for more details.

**You should have received a copy of the GNU Affero General Public License
**along with AliNix. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @abstraction:
 *  - kernel buffer driver for IPV4 implemented.
*/
#include <net/pbuf.h>
#include <alinix/types.h>
#include <net/opt.h>
#include <net/debug.h>
#include <alinix/memory.h>
#include <alinix/compiler.h>
#include <net/mem.h>
#include <net/ip_frag.h>
#include <net/perf.h>
#include <net/debug.h>
#include <net/err.h>
#include <net/pbuf.h>
#include <alinix/module.h>

MODULE_AUTHOR("Ali Mirmohammad")
MODULE_DESCRIPTION("Kernel Buffer Driver for IPV4")
MODULE_LICENSE("AGPL")
MODULE_VERSION("0.1")


#define PBUF_POOL_BUFSIZE_ALIGNED LWIP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE)
#define SIZEOF_STRUCT_PBUF        LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))


#define PBUF_POOL_IS_EMPTY() pbuf_pool_is_empty()
volatile uint8_t pbuf_free_ooseq_pending;





/**
 * Check if the pbuf pool is empty.
 *
 * @note This function checks if the pbuf pool is empty and performs the following actions:
 *       - If the pbuf pool is empty, it sets the pbuf_free_ooseq_pending flag to 1.
 *       - If the pbuf pool is not empty, it sets the pbuf_free_ooseq_pending flag to 1 and queues a call to pbuf_free_ooseq if not already queued.
 *
 * @see pbuf_pool_is_empty
 */
static void
pbuf_pool_is_empty(void)
{
#ifndef PBUF_POOL_FREE_OOSEQ_QUEUE_CALL
  pbuf_free_ooseq_pending = 1;
#else /* PBUF_POOL_FREE_OOSEQ_QUEUE_CALL */
  uint8_t queued;
  queued = pbuf_free_ooseq_pending;
  pbuf_free_ooseq_pending = 1;

  if(!queued) {
    /* queue a call to pbuf_free_ooseq if not already queued */
    // PBUF_POOL_FREE_OOSEQ_QUEUE_CALL();
    ;
  }
#endif /* PBUF_POOL_FREE_OOSEQ_QUEUE_CALL */
}




/**
 * Allocates a pbuf chain of pbufs for storing packets.
 *
 * @param layer The layer of the protocol (e.g., PBUF_TRANSPORT, PBUF_IP, PBUF_LINK, PBUF_RAW).
 * @param length The length of the pbuf chain.
 * @param type The type of pbuf (e.g., PBUF_POOL, PBUF_RAM, PBUF_ROM, PBUF_REF).
 *
 * @return A pointer to the allocated pbuf chain, or NULL if allocation fails.
 *
 * @note The function allocates a chain of pbufs of the requested size. The first pbuf
 *       in the chain points to a buffer of data that is at least large enough to hold
 *       the requested amount of data. The remaining pbufs in the chain arept to their
 *       own separate buffers of data. The pbufs are allocated from the pbuf pool.
 *
 * @note The function sets the payload pointer of each pbuf in the chain to point to the
 *       appropriate location within the pbuf's buffer. The length and total length of
 *       each pbuf in the chain are set according to the requested size. The reference
 *       count of each pbuf is set to 1.
 *
 * @note If the requested type is PBUF_POOL, the function allocates a chain of pbufs
 *       from the pbuf pool. If the allocation fails, the function frees the chain
 *       already allocated and returns NULL.
 *
 * @note If the requested type is PBUF_RAM, the function allocates memory for the pbuf
 *       structure and the payload buffer in one contiguous block. If the allocation fails,
 *       the function returns NULL.
 *
 * @note If the requested type is PBUF_ROM or PBUF_REF, the function allocates memory
 *       only for the pbuf structure and sets the payload pointer to NULL. If the allocation
 *       fails, the function returns NULL.
 *
 * @note The function sets the type, flags, reference count, and payload pointer of each
 *       pbuf in the chain accordingly. The function also sets the next pointer of each pbuf
 *       to point to the next pbuf in the chain, except for the last pbuf, which has its next
 *       pointer set to NULL.
 *
 * @note The function returns a pointer to the allocated pbuf chain, or NULL if the
 *       allocation fails.
 *
 * @see pbuf
 */
struct pbuf *
pbuf_alloc(pbuf_layer layer, uint16_t length, pbuf_type type)
{
  struct pbuf *p, *q, *r;
  uint16_t offset;
  uint32_t rem_len; /* remaining length */
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc(length=%"U16_F")\n", length));

  /* determine header offset */
  switch (layer) {
  case PBUF_TRANSPORT:
    /* add room for transport (often TCP) layer header */
    offset = PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN;
    break;
  case PBUF_IP:
    /* add room for IP layer header */
    offset = PBUF_LINK_HLEN + PBUF_IP_HLEN;
    break;
  case PBUF_LINK:
    /* add room for link layer header */
    offset = PBUF_LINK_HLEN;
    break;
  case PBUF_RAW:
    offset = 0;
    break;
  default:
    LWIP_ASSERT("pbuf_alloc: bad pbuf layer", 0);
    return NULL;
  }

  switch (type) {
  case PBUF_POOL:
    /* allocate head of pbuf chain into p */
    p = (struct pbuf *)memp_malloc(MEM_DEBUG);
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc: allocated pbuf %p\n", (void *)p));
    if (p == NULL) {
      PBUF_POOL_IS_EMPTY();
      return NULL;
    }
    p->type = type;
    p->next = NULL;

    /* make the payload pointer point 'offset' bytes into pbuf data memory */
    p->payload = LWIP_MEM_ALIGN((void *)((uint8_t *)p + (SIZEOF_STRUCT_PBUF + offset)));
    LWIP_ASSERT("pbuf_alloc: pbuf p->payload properly aligned",
            ((mem_ptr_t)p->payload % MEM_ALIGNMENT) == 0);
    /* the total length of the pbuf chain is the requested size */
    p->tot_len = length;
    /* set the length of the first pbuf in the chain */
    p->len = LWIP_MIN(length, PBUF_POOL_BUFSIZE_ALIGNED - LWIP_MEM_ALIGN_SIZE(offset));
    LWIP_ASSERT("check p->payload + p->len does not overflow pbuf",
                ((uint8_t*)p->payload + p->len <=
                 (uint8_t*)p + SIZEOF_STRUCT_PBUF + PBUF_POOL_BUFSIZE_ALIGNED));
    LWIP_ASSERT("PBUF_POOL_BUFSIZE must be bigger than MEM_ALIGNMENT",
      (PBUF_POOL_BUFSIZE_ALIGNED - LWIP_MEM_ALIGN_SIZE(offset)) > 0 );
    /* set reference count (needed here in case we fail) */
    p->ref = 1;

    /* now allocate the tail of the pbuf chain */

    /* remember first pbuf for linkage in next iteration */
    r = p;
    /* remaining length to be allocated */
    rem_len = length - p->len;
    /* any remaining pbufs to be allocated? */
    while (rem_len > 0) {
      q = (struct pbuf *)memp_malloc(MEMP_DEBUG);
      if (q == NULL) {
        PBUF_POOL_IS_EMPTY();
        /* free chain so far allocated */
        pbuf_free(p);
        /* bail out unsuccesfully */
        return NULL;
      }
      q->type = type;
      q->flags = 0;
      q->next = NULL;
      /* make previous pbuf point to this pbuf */
      r->next = q;
      /* set total length of this pbuf and next in chain */
      LWIP_ASSERT("rem_len < max_uint16_t", rem_len < 0xffff);
      q->tot_len = (uint16_t)rem_len;
      /* this pbuf length is pool size, unless smaller sized tail */
      q->len = LWIP_MIN((uint16_t)rem_len, PBUF_POOL_BUFSIZE_ALIGNED);
      q->payload = (void *)((uint8_t *)q + SIZEOF_STRUCT_PBUF);
      LWIP_ASSERT("pbuf_alloc: pbuf q->payload properly aligned",
              ((mem_ptr_t)q->payload % MEM_ALIGNMENT) == 0);
      LWIP_ASSERT("check p->payload + p->len does not overflow pbuf",
                  ((uint8_t*)p->payload + p->len <=
                   (uint8_t*)p + SIZEOF_STRUCT_PBUF + PBUF_POOL_BUFSIZE_ALIGNED));
      q->ref = 1;
      /* calculate remaining length to be allocated */
      rem_len -= q->len;
      /* remember this pbuf for linkage in next iteration */
      r = q;
    }
    /* end of chain */
    /*r->next = NULL;*/

    break;
  case PBUF_RAM:
    /* If pbuf is to be allocated in RAM, allocate memory for it. */
    p = (struct pbuf*)mem_malloc(LWIP_MEM_ALIGN_SIZE(SIZEOF_STRUCT_PBUF + offset) + LWIP_MEM_ALIGN_SIZE(length));
    if (p == NULL) {
      return NULL;
    }
    /* Set up internal structure of the pbuf. */
    p->payload = LWIP_MEM_ALIGN((void *)((uint8_t *)p + SIZEOF_STRUCT_PBUF + offset));
    p->len = p->tot_len = length;
    p->next = NULL;
    p->type = type;

    LWIP_ASSERT("pbuf_alloc: pbuf->payload properly aligned",
           ((mem_ptr_t)p->payload % MEM_ALIGNMENT) == 0);
    break;
  /* pbuf references existing (non-volatile static constant) ROM payload? */
  case PBUF_ROM:
  /* pbuf references existing (externally allocated) RAM payload? */
  case PBUF_REF:
    /* only allocate memory for the pbuf structure */
    p = (struct pbuf *)memp_malloc(MEM_DEBUG);
    if (p == NULL) {
      LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                  ("pbuf_alloc: Could not allocate MEMP_PBUF for PBUF_%s.\n",
                  (type == PBUF_ROM) ? "ROM" : "REF"));
      return NULL;
    }
    /* caller must set this field properly, afterwards */
    p->payload = NULL;
    p->len = p->tot_len = length;
    p->next = NULL;
    p->type = type;
    break;
  default:
    LWIP_ASSERT("pbuf_alloc: erroneous type", 0);
    return NULL;
  }
  /* set reference count */
  p->ref = 1;
  /* set flags */
  p->flags = 0;
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc(length=%"U16_F") == %p\n", length, (void *)p));
  return p;
}

/**
 * Reallocates a pbuf chain to a new length.
 *
 * @param p The pbuf chain to be reallocated.
 * @param new_len The new length of the pbuf chain.
 *
 * @note The function reallocates a pbuf chain to a new length. It first steps over any pbufs
 *       that should remain in the chain, adjusting their length and total length indicators.
 *       Then, it reallocates and adjusts the length of the pbuf that will be split, if necessary.
 *       Finally, it adjusts the length fields for the new last pbuf and frees any remaining
 *       pbufs in the chain.
 *
 * @note The function does not currently support shrinking the pbuf chain.
 *
 * @note The function assumes that the pbuf chain is of type PBUF_POOL, PBUF_ROM, PBUF_RAM,
 *       or PBUF_REF.
 *
 * @note The function assumes that the pbuf chain is properly linked.
 *
 * @pre The pbuf chain must be properly initialized and linked.
 *
 * @post The pbuf chain is reallocated to the new length.
 *
 * @see pbuf
 */
void
pbuf_realloc(struct pbuf *p, uint16_t new_len)
{
  struct pbuf *q;
  uint16_t rem_len; /* remaining length */
  sint32_t grow;

  LWIP_ASSERT("pbuf_realloc: p != NULL", p != NULL);
  LWIP_ASSERT("pbuf_realloc: sane p->type", p->type == PBUF_POOL ||
              p->type == PBUF_ROM ||
              p->type == PBUF_RAM ||
              p->type == PBUF_REF);

  /* desired length larger than current length? */
  if (new_len >= p->tot_len) {
    /* enlarging not yet supported */
    return;
  }

  /* the pbuf chain grows by (new_len - p->tot_len) bytes
   * (which may be negative in case of shrinking) */
  grow = new_len - p->tot_len;

  /* first, step over any pbufs that should remain in the chain */
  rem_len = new_len;
  q = p;
  /* should this pbuf be kept? */
  while (rem_len > q->len) {
    /* decrease remaining length by pbuf length */
    rem_len -= q->len;
    /* decrease total length indicator */
    LWIP_ASSERT("grow < max_uint16_t", grow < 0xffff);
    q->tot_len += (uint16_t)grow;
    /* proceed to next pbuf in chain */
    q = q->next;
    LWIP_ASSERT("pbuf_realloc: q != NULL", q != NULL);
  }
  /* we have now reached the new last pbuf (in q) */
  /* rem_len == desired length for pbuf q */

  /* shrink allocated memory for PBUF_RAM */
  /* (other types merely adjust their length fields */
  if ((q->type == PBUF_RAM) && (rem_len != q->len)) {
    /* reallocate and adjust the length of the pbuf that will be split */
    q = (struct pbuf *)mem_trim(q, (uint16_t)((uint8_t *)q->payload - (uint8_t *)q) + rem_len);
    LWIP_ASSERT("mem_trim returned q == NULL", q != NULL);
  }
  /* adjust length fields for new last pbuf */
  q->len = rem_len;
  q->tot_len = q->len;

  /* any remaining pbufs in chain? */
  if (q->next != NULL) {
    /* free remaining pbufs in chain */
    pbuf_free(q->next);
  }
  /* q is last packet in chain */
  q->next = NULL;

}

uint16_t
pbuf_copy_partial(struct pbuf *buf, void *dataptr, uint16_t len, uint16_t offset)
{
  struct pbuf *p;
  uint16_t left;
  uint16_t buf_copy_len;
  uint16_t copied_total = 0;

  LWIP_ERROR("pbuf_copy_partial: invalid buf", (buf != NULL), return 0;);
  LWIP_ERROR("pbuf_copy_partial: invalid dataptr", (dataptr != NULL), return 0;);

  left = 0;

  if((buf == NULL) || (dataptr == NULL)) {
    return 0;
  }

  /* Note some systems use byte copy if dataptr or one of the pbuf payload pointers are unaligned. */
  for(p = buf; len != 0 && p != NULL; p = p->next) {
    if ((offset != 0) && (offset >= p->len)) {
      /* don't copy from this buffer -> on to the next */
      offset -= p->len;
    } else {
      /* copy from this buffer. maybe only partially. */
      buf_copy_len = p->len - offset;
      if (buf_copy_len > len)
          buf_copy_len = len;
      /* copy the necessary parts of the buffer */
      MEMCPY(&((char*)dataptr)[left], &((char*)p->payload)[offset], buf_copy_len);
      copied_total += buf_copy_len;
      left += buf_copy_len;
      len -= buf_copy_len;
      offset = 0;
    }
  }
  return copied_total;
}

/**
 * Copies a partial portion of a pbuf chain to a data pointer.
 *
 * @param buf The pbuf chain to be copied from.
 * @param dataptr The data pointer to copy to.
 * @param len The length of the data to be copied.
 * @param offset The offset into the pbuf chain from which to start copying.
 *
 * @return The total number of bytes copied.
 *
 * @note The function copies a partial portion of a pbuf chain to a data pointer. It iterates
 *       over the pbuf chain, copying data from each pbuf until the specified length is reached.
 *       The function supports copying from an offset within each pbuf.
 *
 * @note The function assumes that the pbuf chain is properly initialized and linked.
 *
 * @pre The pbuf chain must be properly initialized and linked.
 *
 * @post The specified portion of the pbuf chain is copied to the data pointer.
 *
 * @see pbuf
 */
err_t
pbuf_copy(struct pbuf *p_to, struct pbuf *p_from)
{
  uint16_t offset_to=0, offset_from=0, len;

  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_copy(%p, %p)\n",
    (void*)p_to, (void*)p_from));

  /* is the target big enough to hold the source? */
  LWIP_ERROR("pbuf_copy: target not big enough to hold source", ((p_to != NULL) &&
             (p_from != NULL) && (p_to->tot_len >= p_from->tot_len)), return ERR_ARG;);

  /* iterate through pbuf chain */
  do
  {
    /* copy one part of the original chain */
    if ((p_to->len - offset_to) >= (p_from->len - offset_from)) {
      /* complete current p_from fits into current p_to */
      len = p_from->len - offset_from;
    } else {
      /* current p_from does not fit into current p_to */
      len = p_to->len - offset_to;
    }
    MEMCPY((uint8_t*)p_to->payload + offset_to, (uint8_t*)p_from->payload + offset_from, len);
    offset_to += len;
    offset_from += len;
    LWIP_ASSERT("offset_to <= p_to->len", offset_to <= p_to->len);
    LWIP_ASSERT("offset_from <= p_from->len", offset_from <= p_from->len);
    if (offset_from >= p_from->len) {
      /* on to next p_from (if any) */
      offset_from = 0;
      p_from = p_from->next;
    }
    if (offset_to == p_to->len) {
      /* on to next p_to (if any) */
      offset_to = 0;
      p_to = p_to->next;
      LWIP_ERROR("p_to != NULL", (p_to != NULL) || (p_from == NULL) , return ERR_ARG;);
    }

    if((p_from != NULL) && (p_from->len == p_from->tot_len)) {
      /* don't copy more than one packet! */
      LWIP_ERROR("pbuf_copy() does not allow packet queues!\n",
                 (p_from->next == NULL), return ERR_VAL;);
    }
    if((p_to != NULL) && (p_to->len == p_to->tot_len)) {
      /* don't copy more than one packet! */
      LWIP_ERROR("pbuf_copy() does not allow packet queues!\n",
                  (p_to->next == NULL), return ERR_VAL;);
    }
  } while (p_from);
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_copy: end of chain reached.\n"));
  return ERR_OK;
}

/**
 * Increments the reference count of a pbuf.
 *
 * @param p The pbuf whose reference count is to be incremented.
 *
 * @note The function increments the reference count of a pbuf. This is used to indicate that
 *       the pbuf is being used or referenced by another part of the code.
 *
 * @pre The pbuf must be a valid pointer.
 *
 * @post The reference count of the pbuf is incremented.
 *
 * @see pbuf
 */
void
pbuf_ref(struct pbuf *p)
{
  /* pbuf given? */
  if (p != NULL) {
    ++(p->ref);
  }
}
