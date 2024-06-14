# `v2`

`v2` (also known as **Villamer and Valencia**) is a programming language designed specifically for configuration created and developed by [Cyril John Magayaga](https://github.com/magayaga). It was written in C programming language and can be **application settings**, **user preferences**, **data analysis**, and **more**. It was available for the **Windows**, **Linux**, and **macOS** operating systems.

## Getting Start
You can download the `GCC` compiler (version 9 or above) or `Clang` compiler (version 12 or above) for the **Windows**, **Linux**, and **macOS** operating systems. Then, Build and run the application. 

```bash
# Windows, Linux, or macOS
$ gcc src/v2.c -o v2
```

To run code in a file non-interactively, you can give it as the first argument to the `v2` command:

```bash
./v2 examples/name.v2
```

## Example code

**Generate any static configuration format** — Define all your data in `v2`, and generate output for `JSON`, `YAML`, and other configuration formats. It resembled the [**Apple Pkl**](https://pkl-lang.org).

Example of `name.v2`:
```
name = "Philip II of Spain"

born {
   birthDate = "21 May 1527"
   birthPlace = "Palacio de Pimentel, Valladolid, Crown of Castile"
}

death {
   dateOfDeath = "13 September 1598"
   placeOfDeath = "El Escorial, San Lorenzo de El Escorial, Crown of Castile"
}
```

## Copyright

Copyright (c) 2024 Cyril John Magayaga.
