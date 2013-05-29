Some notes about code style:

The most immediate thing you will notice is that, unlike the wider
lg/scriptlib/publicscript code by Telliamed, this code deliberately avoids
Hungarian Notation as much as possible. If you want, consider it a way of
clearly separating the TW* code from the larger codebase (or maybe I just
utterly loathe Hungarian Notation with the burning passion of a thousand
newborn stars. Which is it? WE MAY NEVER KNOW!)

My code also uses underscores rather than CamelCase except for class names,
and functions imported from outside my code. This is partly another personal
style thing, but in this case there really is the intention of marking code
separation: it makes it easier for me to keep track of what's mine and what
comes from Outside.
