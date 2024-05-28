#include "MTLComputeBuffer.hpp"
#include "MTLComputeGlobals.hpp"
#include "MTLComputeKernel.hpp"
#include "MTLComputeTexture.hpp"
#include <iostream>
#include <vector>

#pragma once

namespace MTLCompute {

    template< typename T>
    class CommandManager {

        private:
            MTL::Device *gpu; ///< The Metal device object
            Kernel *kernel; ///< The kernel object
            MTL::ComputePipelineState *pipeline; ///< The Metal compute pipeline state object
            MTL::CommandQueue *commandQueue; ///< The Metal command queue object
            MTL::CommandBuffer *commandBuffer; ///< The Metal command buffer object
            MTL::ComputeCommandEncoder *commandEncoder; ///< The Metal compute command encoder object
            int arraysize = 50; ///< The size of the buffer and texture arrays
            std::vector<Buffer<T>> buffers = std::vector<Buffer<T>>(arraysize); ///< The buffers
            std::vector<Texture<T>> textures = std::vector<Texture<T>>(arraysize); ///< The textures
            int bufferlength = -1; ///< The length of the buffers
            int texwidth = -1; ///< The width of the textures
            int texheight = -1; ///< The height of the textures

        public:

            /**
             * @brief Constructor for the CommandManager class
             *
             * Takes in the GPU device and the kernel object and creates a new CommandManager.
             * Also takes in the target buffer type with a template parameter.
             *
             * @param gpu The GPU device
             * @param kernel The kernel object
             *
            */
            CommandManager<T>(MTL::Device *gpu, MTLCompute::Kernel *kernel) {
                this->gpu = gpu;
                this->kernel = kernel;
                this->pipeline = this->kernel->getPLS();

                this->commandQueue = this->gpu->newCommandQueue();

            }

            /**
             * @brief Destructor for the CommandManager class
             *
             * Releases the command queue
             *
            */
            ~CommandManager() {
                //this->commandBuffer->release();
                this->commandQueue->release();
            }

            /**
             * @brief Load a buffer into the CommandManager
             *
             * Takes in a buffer and an index and adds the buffer to an internal array
             *
             * @param buffer The buffer to load
             * @param index The index to load the buffer into
             *
            */
            void loadBuffer(Buffer<T> buffer, int index) {
                if (this->bufferlength == -1) {
                    this->bufferlength = buffer.length;
                } else if (this->bufferlength != buffer.length) {
                    throw std::invalid_argument("Buffer lengths do not match");
                }

                this->buffers[index] = buffer;
            }


            void loadTexture(Texture<T> texture, int index) {
                if (this->texwidth == -1 && this->texheight == -1) {
                    this->texwidth = texture.getWidth();
                    this->texheight = texture.getHeight();
                } else if (this->texwidth != texture.getWidth() || this->texheight != texture.getHeight()) {
                    std::cout << this->texwidth << " " << this->texheight << std::endl;
                    std::cout << texture.getTexture()->width() << " " << texture.getTexture()->height() << std::endl;
                    throw std::invalid_argument("Texture sizes do not match");
                }

                this->textures[index] = texture;
            }

            /**
             * @brief Dispatch the kernel
             *
             * Creates new command buffer and command encoder objects,
             * adds the specified buffers at the correct positons, and dispatches the kernel
             *
            */
            void dispatch() {
                if (this->kernel->getPLS() != this->pipeline) {
                    this->pipeline = this->kernel->getPLS();
                }

                this->commandBuffer = this->commandQueue->commandBuffer();
                this->commandEncoder = this->commandBuffer->computeCommandEncoder();
                this->commandEncoder->setComputePipelineState(this->pipeline);
                bool usingbuffers = false;
                bool usingtextures = false;

                for (int i = 0; i < arraysize; i++) {
                    if (buffers[i].length == this->bufferlength && buffers[i].getBuffer() != nullptr) {
                        this->commandEncoder->setBuffer(buffers[i].getBuffer(), 0, i);
                        usingbuffers = true;
                    }
                    if (textures[i].getWidth() == this->texwidth && textures[i].getHeight() == this->texheight
                            && textures[i].getTexture() != nullptr) {
                        this->commandEncoder->setTexture(textures[i].getTexture(), i);
                        usingtextures = true;
                    }
                }

                // Calculate the grid size and thread group size
                MTL::Size threadsperthreadgroup;
                MTL::Size threadspergrid;

                if (usingbuffers && usingtextures) {
                    threadsperthreadgroup = MTL::Size(this->texwidth, this->texheight, 1);
                    threadspergrid = MTL::Size(this->bufferlength, 1, 1);

                } else if (usingbuffers && !usingtextures) {
                    threadsperthreadgroup = MTL::Size(1, 1, 1);
                    threadspergrid = MTL::Size(this->bufferlength, 1, 1);

                } else if (usingtextures && !usingbuffers) {
                    threadsperthreadgroup = MTL::Size(this->texwidth, this->texheight, 1);
                    threadspergrid = MTL::Size(1, 1, 1);
                    
                } else {
                    throw std::invalid_argument("No buffers or textures loaded");
                }

                this->commandEncoder->dispatchThreadgroups(threadspergrid, threadsperthreadgroup);
                this->commandEncoder->endEncoding();
                this->commandBuffer->commit();
                this->commandBuffer->waitUntilCompleted();

                this->commandEncoder->release();
                this->commandBuffer->release();
            }

            void resetBuffers() {
                this->buffers.clear();
                this->buffers = std::vector<Buffer<T>>(this->arraysize);
            }

            void resetTextures() {
                this->textures.clear();
                this->textures = std::vector<Texture<T>>(this->arraysize);
            }

            void reset() {
                this->resetBuffers();
                this->resetTextures();
            }

            MTL::Device *getGPU() {
                return this->gpu;
            }

            Kernel *getKernel() {
                return this->kernel;
            }

            std::vector<Buffer<T>>& getBuffers() {
                return this->buffers;
            }

            std::vector<Texture<T>>& getTextures() {
                return this->textures;
            }


    };

}