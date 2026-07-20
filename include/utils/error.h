#pragma once

void errorGeneric(const char *error, ...);
_Noreturn void errorLang(const char *filename, int line, int column, const char *error, ...);
