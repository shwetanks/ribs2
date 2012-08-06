/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/

struct TEMPLATE(ds_field,T) {
    T *mem;
    size_t num_elements;
};

static inline int TEMPLATE_FUNC(ds_field,T,free)(struct TEMPLATE(ds_field,T) *dsf);
static inline int TEMPLATE_FUNC(ds_field,T,init)(struct TEMPLATE(ds_field,T) *dsf, const char *filename) {
    TEMPLATE_FUNC(ds_field,T,free)(dsf);
    int fd = open(filename, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return LOGGER_PERROR_FUNC("open: %s", filename), -1;
    struct stat st;
    if (0 > fstat(fd, &st))
        return LOGGER_PERROR_FUNC("fstat %s", filename), close(fd), -1;
    off_t size = st.st_size;
    if (size < (off_t)sizeof(int64_t))
        return LOGGER_ERROR_FUNC("%s, wrong file size: %zd", __FUNCTION__, size), close(fd), -1;
    if (0 != ((size - sizeof(int64_t)) % sizeof(T)))
        return LOGGER_ERROR_FUNC("%s: data is misaligned", filename), close(fd), -1;
    void *mem = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR_FUNC("mmap %s", filename), close(fd), -1;
    close(fd);
    int64_t t = *(int64_t *)mem;
    if (t != DS_TYPE(T))
        return LOGGER_ERROR_FUNC("%s: not the same type ([code] %s != [file] %s)", filename, STRINGIFY(DS_TYPE(T)), ds_type_to_string(t)), munmap(mem, size), -1;
    size -= sizeof(int64_t); /* exclude the header */
    dsf->mem = mem + sizeof(int64_t); /* skip the header */
    dsf->num_elements = size / sizeof(T);
    return 0;
}

static inline int TEMPLATE_FUNC(ds_field,T,free)(struct TEMPLATE(ds_field,T) *dsf) {
    if (!dsf->mem) return 0;
    int res;
    if (0 > (res = munmap(dsf->mem - sizeof(int64_t) /* include the header */, (dsf->num_elements * sizeof(T)) + sizeof(int64_t))))
        LOGGER_PERROR_FUNC("munmap");
    dsf->mem = NULL;
    return res;
}

static inline T TEMPLATE_FUNC(ds_field,T,get)(struct TEMPLATE(ds_field,T) *dsf, size_t index) {
    return dsf->mem[index];
}
