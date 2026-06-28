#pragma once
#include <cstddef>
#include <fstream>
#include <string>
#include <array>
#include <cstdint>

const size_t PAGE_SIZE = 4096;
using PageBuffer = std::array<uint8_t, PAGE_SIZE>;

class Pager {
  public:
    Pager(const std::string& filepath);
    ~Pager();

    Pager(const Pager&) = delete;
    Pager& operator=(const Pager&)=delete;
    
    Pager(Pager&& other) noexcept;
    Pager& operator=(Pager&& other) noexcept;

    PageBuffer readPage(uint32_t pageId);

    void writePage(uint32_t pageId, const PageBuffer& buffer);
    
    uint32_t allocatePage();

    uint32_t getPageCount() const;

private:
    std::fstream file_;
    uint32_t pageCount_;
};
