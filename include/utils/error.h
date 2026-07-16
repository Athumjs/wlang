#pragma once

void errorGeneric(const char *error, ...);
void errorLang(const char *filename, int line, int column, const char *error, ...);
