/* radare - LGPL - Copyright 2008-2014 - pancake, nibble */

#include "r_io.h"

R_API void r_io_section_init(RIO *io) {
	io->next_section_id = 0;
	io->enforce_rwx = 0; // do not enforce RWX section permissions by default
	io->enforce_seek = 0; // do not limit seeks out of the file by default
	io->sections = r_list_new ();
}

#if 0
static int cmpaddr (void *_a, void *_b) {
	RIOSection *a = _a, *b = _b;
	return (a->vaddr > b->vaddr);
}
#endif

R_API RIOSection *r_io_section_get_name(RIO *io, const char *name) {
	RListIter *iter;
	RIOSection *s;
	if (name)
	r_list_foreach (io->sections, iter, s) {
		if (!strcmp (name, s->name))
			return s;
	}
	return NULL;
}

R_API void r_io_section_add(RIO *io, ut64 offset, ut64 vaddr, ut64 size, ut64 vsize, int rwx, const char *name, ut32 bin_id, int fd) {
	int update = 0;
	RIOSection *s;
	if (size==0 || size>0xf0000000) {
		if (size>0 && size != UT64_MAX && size != UT32_MAX)
			eprintf ("Invalid size (0x%08"PFMT64x") for section '%s' at 0x%08"PFMT64x"\n",
			size, name, vaddr);
		return;
	}
	s = r_io_section_get_name (io, name);
	if (s == NULL) {
		s = R_NEW (RIOSection);
		s->id = io->next_section_id++;
	} else update = 1;
	s->offset = offset;
	s->vaddr = vaddr;
	s->size = size;
	s->vsize = vsize;
	s->rwx = rwx;
	s->arch = s->bits = 0;
	s->bin_id = bin_id;
	s->fd = fd;
	if (!update) {
		if (name) strncpy (s->name, name, sizeof (s->name)-4);
		else *s->name = '\0';
		r_list_append (io->sections, s);
	}
}

R_API RIOSection *r_io_section_get_i(RIO *io, int idx) {
	RListIter *iter;
	RIOSection *s;
	r_list_foreach (io->sections, iter, s) {
		if (s->id == idx)
			return s;
	}
	return NULL;
}

R_API int r_io_section_rm(RIO *io, int idx) {
	return r_list_del_n (io->sections, idx);
}

R_API void r_io_section_clear(RIO *io) {
	r_list_free (io->sections);
	io->sections = r_list_new ();
}

// TODO: implement as callback
R_API void r_io_section_list(RIO *io, ut64 offset, int rad) {
	int i = 0;
	RListIter *iter;
	RIOSection *s;

	if (io->va || io->debug)
		offset = r_io_section_vaddr_to_offset (io, offset);
	// XXX - Should this print the section->id or the location in the
	// rio sections array?
	r_list_foreach (io->sections, iter, s) {
		if (rad) {
			char *n = strdup (s->name);
			r_name_filter (n, strlen (n));
			io->printf ("f section.%s %"PFMT64d" 0x%"PFMT64x"\n",
				n, s->size, s->vaddr);
			io->printf ("S 0x%08"PFMT64x" 0x%08"PFMT64x" 0x%08"
				PFMT64x" 0x%08"PFMT64x" %s %s\n", s->offset,
				s->vaddr, s->size, s->vsize, n, r_str_rwx_i (s->rwx));
		} else {
			io->printf ("[%02d] %c 0x%08"PFMT64x" %s va=0x%08"PFMT64x
				" sz=0x%08"PFMT64x" vsz=%08"PFMT64x" %s",
			i, (offset>=s->offset && offset<s->offset+s->size)?'*':'.',
			s->offset, r_str_rwx_i (s->rwx), s->vaddr, s->size, s->vsize, s->name);
			if (s->arch && s->bits)
				io->printf ("  ; %s %d\n", r_sys_arch_str (s->arch), s->bits);
			else io->printf ("\n");
		}
		i++;
	}
}

/* TODO: move to print ??? support pretty print of ranges following an array of offsetof */
R_API void r_io_section_list_visual(RIO *io, ut64 seek, ut64 len) {
	RListIter *iter;
	RIOSection *s;
	ut64 min = -1;
	ut64 max = -1;
	ut64 mul;
	int j, i, width = 30; //config.width-30;

	seek = (io->va || io->debug) ? r_io_section_vaddr_to_offset (io, seek) : seek;
	r_list_foreach (io->sections, iter, s) {
		if (min == -1 || s->offset < min)
			min = s->offset;
		if (max == -1 || s->offset+s->size > max)
			max = s->offset+s->size;
	}

	mul = (max-min) / width;
	if (min != -1 && mul != 0) {
		i = 0;
		r_list_foreach (io->sections, iter, s) {
			io->printf ("%02d%c 0x%08"PFMT64x" |",
				i, (seek>=s->offset && seek<s->offset+s->size)?'*':' ', s->offset);
			for (j=0; j<width; j++) {
				ut64 pos = min + (j*mul);
				ut64 npos = min + ((j+1)*mul);
				if (s->offset <npos && (s->offset+s->size)>pos)
					io->printf ("#");
				else io->printf ("-");
			}
			io->printf ("| 0x%08"PFMT64x" %s %s\n", s->offset+s->size, 
				r_str_rwx_i (s->rwx), s->name);
			i++;
		}
		/* current seek */
		if (i>0 && len != 0) {
			if (seek == UT64_MAX)
				seek = 0;
			//len = 8096;//r_io_size (io);
			io->printf ("=>  0x%08"PFMT64x" |", seek);
			for (j=0;j<width;j++) {
				io->printf (
					((j*mul)+min >= seek &&
					 (j*mul)+min <= seek+len)
					?"^":"-");
			}
			io->printf ("| 0x%08"PFMT64x"\n", seek+len);
		}
	}
}

R_API RIOSection *r_io_section_vget(RIO *io, ut64 vaddr) {
	RListIter *iter;
	RIOSection *s;
	r_list_foreach (io->sections, iter, s) {
		if (vaddr >= s->vaddr && vaddr < s->vaddr + s->vsize)
			return s;
	}
	return NULL;
}

R_API RIOSection *r_io_section_mget(RIO *io, ut64 maddr) {
	RIOSection *s;
	RListIter *iter;
	r_list_foreach (io->sections, iter, s) {
		if ((maddr >= s->offset && maddr < (s->offset + s->size)))
			return s;
	}
	return NULL;
}

// XXX: rename this
R_API ut64 r_io_section_get_offset(RIO *io, ut64 maddr) {
	RIOSection *s = r_io_section_mget(io, maddr);
	return s? s->offset: -1;
}

// XXX: must be renamed, this is confusing
R_API ut64 r_io_section_get_vaddr(RIO *io, ut64 maddr) {
	RIOSection *s = r_io_section_mget (io, maddr);
	return s? s->vaddr: -1;
}

// TODO: deprecate
R_API int r_io_section_get_rwx(RIO *io, ut64 offset) {
	RIOSection *s = r_io_section_mget (io, offset);
	return s?s->rwx:R_IO_READ|R_IO_WRITE|R_IO_EXEC;
}

R_API int r_io_section_overlaps(RIO *io, RIOSection *s) {
	int i = 0;
	RListIter *iter;
	RIOSection *s2;

	r_list_foreach (io->sections, iter, s2) {
		if (s != s2) {
			if (s->offset >= s2->offset) {
				if (s2->offset+s2->size < s->offset)
					return i;
			} else {
				if (s->offset+s->size < s2->offset)
					return i;
			}
		}
		i++;
	}
	return -1;
}

// TODO: rename to r_io_section_vaddr_to_maddr
R_API ut64 r_io_section_vaddr_to_offset(RIO *io, ut64 vaddr) {
	RListIter *iter;
	RIOSection *s;

	r_list_foreach (io->sections, iter, s) {
		if (vaddr >= s->vaddr && vaddr < s->vaddr + s->vsize) {
			/* XXX: Do we need this hack?
			if (s->vaddr == 0) // hack
				return vaddr;
			*/
			return (vaddr - s->vaddr + s->offset);
		}
	}
	return vaddr;
}

// TODO: rename to r_io_section_maddr_to_vaddr
R_API ut64 r_io_section_offset_to_vaddr(RIO *io, ut64 offset) {
	RIOSection *s = r_io_section_vget (io, offset);
	if (s) {
		io->section = s;
		return (s->vaddr + offset - s->offset);
	}
	return UT64_MAX;
}

// TODO: deprecate ?
R_API int r_io_section_exists_for_paddr (RIO *io, ut64 maddr) {
	RIOSection *s = r_io_section_mget (io, maddr);
	return s?1:0;
}

// TODO: deprecate ?
R_API int r_io_section_exists_for_vaddr (RIO *io, ut64 vaddr) {
	RIOSection *s = r_io_section_vget (io, vaddr);
	return s?1:0;
}

// dupped in vio.c
R_API ut64 r_io_section_next(RIO *io, ut64 o) {
	int oset = 0;
	ut64 newsec = 0LL;
	RListIter *iter;
	RIOSection *s;

	r_list_foreach (io->sections, iter, s) {
		ut64 addr = s->vaddr;
		if (o < addr) {
			if (newsec) {
				if (addr<newsec)
					newsec = addr;//s->offset;//addr;
			} else newsec = addr; //s->offset; //addr;
		}
		if (o >= s->offset && o < (s->offset + s->size)) {
			ut64 n = s->offset + s->size;
			if (n>o) {
				o = n;
				oset = 1;
			}
		}
		if (o >= s->vaddr && o < (s->vaddr + s->size)) {
			ut64 n = s->vaddr + s->size;
			if (n>o) {
				o = n;
				oset = 1;
			}
		}
	}
	//eprintf ("Newsec %d %llx\n", oset, newsec);
	if (oset) return o;
	return newsec? newsec: o;
}

R_API RList *r_io_section_get_in_paddr_range(RIO *io, ut64 addr, ut64 endaddr) {
	RIOSection *section;
	RListIter *iter;
	RList *sections = r_list_new ();
	ut64 sec_from, sec_to;
	r_list_foreach (io->sections, iter, section) {
		sec_from = section->offset;
		sec_to = sec_from + section->size;
		if (sec_from <= addr && addr < sec_to) r_list_append(sections, section);
		if (sec_from < endaddr && endaddr < sec_to) r_list_append(sections, section);
		if (addr <= sec_from && sec_to <= endaddr) r_list_append(sections, section);
	}
	return sections;
}

R_API RList *r_io_section_get_in_vaddr_range(RIO *io, ut64 addr, ut64 endaddr) {
	RIOSection *section;
	RListIter *iter;
	RList *sections = r_list_new ();
	ut64 sec_from, sec_to;
	r_list_foreach (io->sections, iter, section) {
		sec_from = section->vaddr;
		sec_to = sec_from + section->vsize;
		if (sec_from <= addr && addr < sec_to) r_list_append(sections, section);
		if (sec_from < endaddr && endaddr < sec_to) r_list_append(sections, section);
		if (addr <= sec_from && sec_to <= endaddr) r_list_append(sections, section);
	}
	return sections;
}

R_API RIOSection * r_io_section_get_first_in_paddr_range(RIO *io, ut64 addr, ut64 endaddr) {
	RIOSection *sec = NULL;
	RListIter *iter;
	ut64 sec_from, sec_to;
	r_list_foreach (io->sections, iter, sec) {
		sec_to = sec->offset + sec->size;
		sec_from = sec->offset;
		if (sec_from <= addr && addr < sec_to) break;
		//if (map->from == addr && endaddr == sec_to) r_list_append(maps, map);
		if (sec_from < endaddr && endaddr < sec_to) break;
		if (addr <= sec_from && sec_to <= endaddr) break;
		sec = NULL;
	}
	return sec;
}

R_API RIOSection * r_io_section_get_first_in_vaddr_range(RIO *io, ut64 addr, ut64 endaddr) {
	RIOSection *sec = NULL;
	RListIter *iter;
	ut64 sec_from, sec_to;
	r_list_foreach (io->sections, iter, sec) {
		sec_to = sec->vaddr + sec->vsize;
		sec_from = sec->vaddr;
		if (sec_from <= addr && addr < sec_to) break;
		//if (map->from == addr && endaddr == sec_to) r_list_append(maps, map);
		if (sec_from < endaddr && endaddr < sec_to) break;
		if (addr <= sec_from && sec_to <= endaddr) break;
		sec = NULL;
	}
	return sec;
}

R_API int r_io_section_set_archbits(RIO *io, ut64 addr, const char *arch, int bits) {
	RIOSection *s = r_io_section_vget (io, addr);
	if (!s) return R_FALSE;
	if (arch) {
		s->arch = r_sys_arch_id (arch);
		s->bits = bits;
	} else {
		s->arch = 0;
		s->bits = 0;
	}
	return R_TRUE;
}

R_API const char *r_io_section_get_archbits(RIO* io, ut64 addr, int *bits) {
	RIOSection *s = r_io_section_vget (io, addr);
	if (!s || !s->bits || !s->arch) return NULL;
	if (bits) *bits = s->bits;
	return r_sys_arch_str (s->arch);
}

R_API RIOSection *r_io_section_getv_bin_id(RIO *io, ut64 vaddr, ut32 bin_id) {
	RListIter *iter;
	RIOSection *s;
	r_list_foreach (io->sections, iter, s) {
		if (s->bin_id != bin_id) continue;
		if (vaddr >= s->vaddr && vaddr < s->vaddr + s->vsize)
			return s;
	}
	return NULL;
}

R_API int r_io_section_set_archbits_bin_id(RIO *io, ut64 addr, const char *arch, int bits, ut32 bin_id) {
	RIOSection *s = r_io_section_getv_bin_id (io, addr, bin_id);
	if (!s) return R_FALSE;
	if (arch) {
		s->arch = r_sys_arch_id (arch);
		s->bits = bits;
	} else {
		s->arch = 0;
		s->bits = 0;
	}
	return R_TRUE;
}
