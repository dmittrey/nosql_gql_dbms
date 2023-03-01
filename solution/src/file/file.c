#include "memory/file/file.h"

static sect_ext_t *get_sect_ext(const file_t *const file, fileoff_t fileoff);
static sect_ext_t *find_sect_ext(const file_t *const file, size_t entity_size);

file_t *file_new()
{
    return my_malloc(file_t);
}

void file_ctor(file_t *const file, FILE *const filp)
{
    file->header.lst_sect_ptr = sizeof(file_head_t);
    file->f_extent = NULL;
    file->filp = filp;
}
void file_dtor(file_t *file)
{
    sect_ext_t *cur_extent = file->f_extent;
    sect_ext_t *next_extent = NULL;
    while (cur_extent != NULL)
    {
        next_extent = cur_extent->next;
        sect_ext_dtor(cur_extent);
        cur_extent = next_extent;
    }

    free(file);
}

/*
1) Запустили запись от брата
2) Запустили запись от сына
3) Нашли секцию в которой есть пространство для записи
    Если не нашли то добавили в конец файла новую секцию
5) Записали адрес текущей сущности в записанного брата
6) Записали адрес текущей сущности в записанного сына
*/
status_t file_write(file_t *const file, const json_t *const json, fileoff_t dad_fileoff, fileoff_t *const write_addr)
{
    ENTITY_INIT(entity, json, dad_fileoff, 0, 0);

    fileoff_t bro_offset = 0;
    if (json->bro != NULL)
    {
        DO_OR_FAIL(file_write(file, json->bro, 0, &bro_offset));
        entity->fam_addr.bro_ptr = bro_offset;
    }

    fileoff_t son_offset = 0;
    if (json->son != NULL)
    {
        DO_OR_FAIL(file_write(file, json->son, 0, &son_offset));
        entity->fam_addr.son_ptr = son_offset;
    }

    sect_ext_t *extents = find_sect_ext(file, entity_ph_size(entity));
    if (extents == NULL)
    {
        extents = sect_ext_new();
        DO_OR_FAIL(file_add_sect_ext(file, extents));
    }

    sect_ext_write(extents, json, entity, write_addr);
    *write_addr += extents->header.sect_off;

    if (son_offset)
    {
        RA_FWRITE_OR_FAIL(write_addr, sizeof(write_addr), son_offset + offsetof(entity_t, fam_addr) + offsetof(tplgy_addr, dad_ptr), file->filp);
    }

    entity_dtor(entity);

    return OK;
}
/*
1) Прочитать секцию с нодой
2) Прочитать ноду по секционному смещению
3) Если есть брат, запуск от брата
4) Прокинуть ссылку на брата
5) Если есть сын, запуск от сына
6) Прокинуть ссылку на сына
*/
status_t file_read(file_t *const file, const fileoff_t fileoff, json_t *const ret_json)
{

    sect_ext_t *extents = get_sect_ext(file, fileoff);
    if (extents == NULL) return FAILED;

    entity_t *ret_entity = entity_new();
    DO_OR_FAIL(sect_ext_read(extents, sect_head_get_sectoff(&extents->header, fileoff), ret_entity, ret_json));

    if (ret_entity->fam_addr.bro_ptr != 0)
    {
        json_t *bro_json = json_new();
        DO_OR_FAIL(file_read(file, ret_entity->fam_addr.bro_ptr, bro_json));
        ret_json->bro = bro_json;
    }

    if (ret_entity->fam_addr.son_ptr != 0)
    {
        json_t *son_json = json_new();
        DO_OR_FAIL(file_read(file, ret_entity->fam_addr.son_ptr, son_json));
        ret_json->son = son_json;
    }

    return OK;
}
status_t file_update(file_t *const file, const fileoff_t fileoff, const json_t *const new_json);
status_t file_delete(file_t *const file, const fileoff_t fileoff);

/*
Add empty extents section to end of file
*/
status_t file_add_sect_ext(file_t *const file, sect_ext_t *new_extents)
{
    DO_OR_FAIL(sect_ext_ctor(new_extents, file->header.lst_sect_ptr, file->filp));
    file->header.lst_sect_ptr += SECTION_SIZE;
    if (file->f_extent == NULL)
    {
        file->f_extent = new_extents;
        return OK;
    }

    sect_ext_t *cur = file->f_extent;
    while (cur->next != NULL)
    {
        cur = cur->next;
    }
    cur->next = new_extents;
    cur->header.next_ptr = new_extents->header.sect_off;
    sect_ext_sync(cur);
    return OK;
}
status_t file_read_sect_ext(file_t *const file, const fileoff_t fileoff, sect_ext_t *const r_sect_ext)
{
    RA_FREAD_OR_FAIL(r_sect_ext, sizeof(r_sect_ext), fileoff, file->filp);
    return OK;
}

static sect_ext_t *get_sect_ext(const file_t *const file, fileoff_t fileoff)
{
    sect_ext_t *r_sect_ext = file->f_extent;

    while (r_sect_ext != NULL)
    {
        if (r_sect_ext->header.sect_off <= fileoff && r_sect_ext->header.sect_off + SECTION_SIZE > fileoff)
        {
            break;
        }

        r_sect_ext = r_sect_ext->next;
    }

    return r_sect_ext;
}
static sect_ext_t *find_sect_ext(const file_t *const file, size_t entity_size)
{
    sect_ext_t *cur_ext = file->f_extent;
    while (cur_ext != NULL && cur_ext->header.free_space < entity_size)
    {
        cur_ext = cur_ext->next;
    }

    return cur_ext;
}