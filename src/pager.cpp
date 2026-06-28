#include "pager.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <iostream>

Pager::Pager(const std::string& filepath) {
  // If the file doesn't exist, create an empty one first
    if (!std::filesystem::exists(filepath)) {
        std::ofstream create_file(filepath, std::ios::binary);
        create_file.close();
    }

    file_.open(filepath, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
      throw std::runtime_error("Failed to open index file: " + filepath);
    }
    
    // Find the file size
    file_.seekg(0, std::ios::end);
    std::streampos fileSize = file_.tellg();
    // Check for corrupt size
    if (fileSize % PAGE_SIZE != 0) {
        throw std::runtime_error("Index file size is not a multiple of PAGE_SIZE: " + filepath);
    }
    pageCount_ = fileSize / PAGE_SIZE;

}

Pager::~Pager() {
  if (file_.is_open())
    file_.close();
}

Pager::Pager(Pager&& other) noexcept
    : file_(std::move(other.file_)), pageCount_(other.pageCount_) {
    other.pageCount_ = 0;
}

Pager& Pager::operator=(Pager&& other) noexcept {
    if (this != &other) {
        if (file_.is_open()) file_.close();
        file_ = std::move(other.file_);
        pageCount_ = other.pageCount_;
        other.pageCount_ = 0;
    }
    return *this;
}

PageBuffer Pager::readPage(uint32_t pageId) {
  if (pageId>=pageCount_)
    throw std::out_of_range("Page ID " + std::to_string(pageId) + " is out of bounds.");
  PageBuffer buffer;
  file_.seekg(pageId * PAGE_SIZE, std::ios::beg);
  file_.read(reinterpret_cast<char*>(buffer.data()), PAGE_SIZE);
  return buffer;
}

void Pager::writePage(uint32_t pageId, const PageBuffer& buffer) {
  if (pageId >= pageCount_) {
        throw std::out_of_range("Page ID " + std::to_string(pageId) + " is out of bounds.");
  }
  file_.seekp(pageId * PAGE_SIZE, std::ios::beg);
  file_.write(reinterpret_cast<const char*>(buffer.data()), PAGE_SIZE);
  file_.flush(); // Force write to physical disk
}

uint32_t Pager::allocatePage() {
  uint32_t newPageId = pageCount_;
  PageBuffer blank={};

  file_.seekp(newPageId * PAGE_SIZE, std::ios::beg);
  file_.write(reinterpret_cast<const char*>(blank.data()), PAGE_SIZE);
  file_.flush();

  pageCount_++;
  return newPageId;
}

uint32_t Pager::getPageCount() const {
  return pageCount_;
}


