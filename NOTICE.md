# External sources and current scope

The foundation uses REFramework's official C API header at commit
`d052e8fd3fd41d0a1a33394fcf6e6c19be39b1ee`. Its MIT notice is in
`licenses/REFramework.txt`.

Lua 5.4.3 source is downloaded from the official archive and hash-checked.
It is reserved for the settings UI; game lighting is not implemented in Lua.
Its MIT notice is in `licenses/Lua.txt`.

MinHook `c3fcafdc10146beb5919319d0683e44e3c30d537` is planned but is not
downloaded or linked in this foundation. Its license and file hashes must
be added before it is incorporated.

The RE7 OTS implementation at `ffde226b12c76e3dfec806e8e0a23de267e2b6a5`
is a research reference. No RE7 game addresses, CT archive, camera/model
code or game assets are copied into this build. Original OTS authors:
cheezeit / goldedtoad. RE7 implementation: like-shock/re7-ots and contributors.

This project is an unofficial mod and is not affiliated with Capcom.
