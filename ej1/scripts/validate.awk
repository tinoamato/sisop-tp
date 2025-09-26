BEGIN {
    FS=","            # separador CSV
    getline           # saltear cabecera
}
{
    if (NR == 1) next
    id = $1
    if (id_seen[id]++) {
        print "Error: ID duplicado " id
        exit 1
    }
    max_id = (id > max_id ? id : max_id)
}
END {
    for (i = 1; i <= max_id; i++) {
        if (!(i in id_seen)) {
            print "Error: falta ID " i
            exit 1
        }
    }
    print "Validación OK: IDs correlativos y sin duplicados (orden indiferente)."
}