# libSceVideoCoreInterface technical debt

### Unknown function info

- [ and sceVideoCoreIfSetFunctionTableUnlimited](../../../core/libs/prx/libSceVideoCoreInterface/Export.cpp) (libSceVideoCoreInterface) - names and NIDs are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceVideoCoreInterface.c), but no credible prototypes were found. The declarations have no parameters solely to resolve known imports; this is not an ABI claim and callers must not call them until signatures are verified
