/* body of pstrcmp: the head is in engineutils.c at both of its positions */
{
    short i;

    if (a[0] != b[0])
        return -1;
    for (i = 0; i < (a[0] && b[0]); i++) {
        if (a[i] != b[i])
            return -1;
    }
    return 0;
}
