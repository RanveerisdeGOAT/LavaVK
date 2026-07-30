//
// Created by paikr on 7/30/2026.
//

#ifndef LAVAVK_ERROR_HPP
#define LAVAVK_ERROR_HPP
#ifndef LAVAVK_ERROR_HANDLER
#define LAVAVK_ERROR_HANDLER
#ifndef NDEBUG
#define LAVAVK_ERROR(msg) throw std::runtime_error(msg)
#else
#define LAVAVK_ERROR(msg) std::cerr << msg << std::endl
#endif
#endif
#endif //LAVAVK_ERROR_HPP
