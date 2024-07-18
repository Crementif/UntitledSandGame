#pragma once

template<typename Type>
void writeAt(std::vector<uint8_t>& fh, size_t pos, Type value) {
    *reinterpret_cast<Type*>(fh.data() + pos) = value;
}

template<typename Type>
void write(std::vector<uint8_t>& fh, Type value) {
    auto pos = fh.size();
    fh.resize(pos + sizeof(Type));
    *reinterpret_cast<Type*>(fh.data() + pos) = value;
}

void writeString(std::vector<uint8_t>& fh, const char* str) {
    auto pos = fh.size();
    auto len = strlen(str) + 1;
    fh.resize((static_cast<size_t>(pos + len) + (4 - 1)) & ~(4 - 1));
    memcpy(fh.data() + pos, str, len);
}

void writeGX2RBuffer(std::vector<uint8_t>& fh, GX2RBuffer* buffer) {
    write(fh, buffer->flags);
    write(fh, buffer->elemSize);
    write(fh, buffer->elemCount);
    auto pos = fh.size();
    fh.resize(pos + buffer->elemSize * buffer->elemCount);
    memcpy(fh.data() + pos, buffer->buffer, buffer->elemSize * buffer->elemCount);
}

std::vector<uint8_t> SerializeVertexShader(GX2VertexShader* vertexShader) {
    std::vector<uint8_t> data;

    // write regs
    write(data, vertexShader->regs.sq_pgm_resources_vs);
    write(data, vertexShader->regs.vgt_primitiveid_en);
    write(data, vertexShader->regs.spi_vs_out_config);
    write(data, vertexShader->regs.num_spi_vs_out_id);
    for (uint32_t spi_vs : vertexShader->regs.spi_vs_out_id) {
        write(data, spi_vs);
    }
    write(data, vertexShader->regs.pa_cl_vs_out_cntl);
    write(data, vertexShader->regs.sq_vtx_semantic_clear);
    write(data, vertexShader->regs.num_sq_vtx_semantic);
    for (uint32_t sq_vtx : vertexShader->regs.sq_vtx_semantic) {
        write(data, sq_vtx);
    }
    write(data, vertexShader->regs.vgt_strmout_buffer_en);
    write(data, vertexShader->regs.vgt_vertex_reuse_block_cntl);
    write(data, vertexShader->regs.vgt_hos_reuse_depth);

    // write program
    write(data, vertexShader->size);
    for (uint32_t i = 0; i < vertexShader->size; i++) {
        write(data, ((uint8_t*)vertexShader->program)[i]);
    }
    write(data, vertexShader->mode);

    // write uniform blocks
    write(data, vertexShader->uniformBlockCount);
    for (uint32_t i = 0; i < vertexShader->uniformBlockCount; i++) {
        writeString(data, vertexShader->uniformBlocks[i].name);
        write(data, vertexShader->uniformBlocks[i].offset);
        write(data, vertexShader->uniformBlocks[i].size);
    }

    // write uniform vars
    write(data, vertexShader->uniformVarCount);
    for (uint32_t i = 0; i < vertexShader->uniformVarCount; i++) {
        writeString(data, vertexShader->uniformVars[i].name);
        write(data, vertexShader->uniformVars[i].type);
        write(data, vertexShader->uniformVars[i].count);
        write(data, vertexShader->uniformVars[i].offset);
        write(data, vertexShader->uniformVars[i].block);
    }

    // write initial values
    write(data, vertexShader->initialValueCount);
    for (uint32_t i = 0; i < vertexShader->initialValueCount; i++) {
        write(data, vertexShader->initialValues[i].value[0]);
        write(data, vertexShader->initialValues[i].value[1]);
        write(data, vertexShader->initialValues[i].value[2]);
        write(data, vertexShader->initialValues[i].value[3]);
        write(data, vertexShader->initialValues[i].offset);
    }

    // write loop vars
    write(data, vertexShader->loopVarCount);
    for (uint32_t i = 0; i < vertexShader->loopVarCount; i++) {
        write(data, vertexShader->loopVars[i].offset);
        write(data, vertexShader->loopVars[i].value);
    }

    // write sampler vars
    write(data, vertexShader->samplerVarCount);
    for (uint32_t i = 0; i < vertexShader->samplerVarCount; i++) {
        writeString(data, vertexShader->samplerVars[i].name);
        write(data, vertexShader->samplerVars[i].type);
        write(data, vertexShader->samplerVars[i].location);
    }

    // write attribute vars
    write(data, vertexShader->attribVarCount);
    for (uint32_t i = 0; i < vertexShader->attribVarCount; i++) {
        writeString(data, vertexShader->attribVars[i].name);
        write(data, vertexShader->attribVars[i].type);
        write(data, vertexShader->attribVars[i].count);
        write(data, vertexShader->attribVars[i].location);
    }

    // write ring item size
    write(data, vertexShader->ringItemsize);

    // write stream out
    write(data, vertexShader->hasStreamOut);
    for (uint32_t stride : vertexShader->streamOutStride) {
        write(data, stride);
    }

    // write gx2rBuffer
    writeGX2RBuffer(data, &vertexShader->gx2rBuffer);

    return data;
}

std::vector<uint8_t> SerializePixelShader(GX2PixelShader* pixelShader) {
    std::vector<uint8_t> data;

    // write regs
    write(data, pixelShader->regs.sq_pgm_resources_ps);
    write(data, pixelShader->regs.sq_pgm_exports_ps);
    write(data, pixelShader->regs.spi_ps_in_control_0);
    write(data, pixelShader->regs.spi_ps_in_control_1);
    write(data, pixelShader->regs.num_spi_ps_input_cntl);
    for (uint32_t spi_ps : pixelShader->regs.spi_ps_input_cntls) {
        write(data, spi_ps);
    }
    write(data, pixelShader->regs.cb_shader_mask);
    write(data, pixelShader->regs.cb_shader_control);
    write(data, pixelShader->regs.db_shader_control);
    write(data, pixelShader->regs.spi_input_z);

    // write program
    write(data, pixelShader->size);
    for (uint32_t i = 0; i < pixelShader->size; i++) {
        write(data, ((uint8_t*)pixelShader->program)[i]);
    }
    write(data, pixelShader->mode);

    // write uniform blocks
    write(data, pixelShader->uniformBlockCount);
    for (uint32_t i = 0; i < pixelShader->uniformBlockCount; i++) {
        writeString(data, pixelShader->uniformBlocks[i].name);
        write(data, pixelShader->uniformBlocks[i].offset);
        write(data, pixelShader->uniformBlocks[i].size);
    }

    // write uniform vars
    write(data, pixelShader->uniformVarCount);
    for (uint32_t i = 0; i < pixelShader->uniformVarCount; i++) {
        writeString(data, pixelShader->uniformVars[i].name);
        write(data, pixelShader->uniformVars[i].type);
        write(data, pixelShader->uniformVars[i].count);
        write(data, pixelShader->uniformVars[i].offset);
        write(data, pixelShader->uniformVars[i].block);
    }

    // write initial values
    write(data, pixelShader->initialValueCount);
    for (uint32_t i = 0; i < pixelShader->initialValueCount; i++) {
        write(data, pixelShader->initialValues[i].value[0]);
        write(data, pixelShader->initialValues[i].value[1]);
        write(data, pixelShader->initialValues[i].value[2]);
        write(data, pixelShader->initialValues[i].value[3]);
        write(data, pixelShader->initialValues[i].offset);
    }

    // write loop vars
    write(data, pixelShader->loopVarCount);
    for (uint32_t i = 0; i < pixelShader->loopVarCount; i++) {
        write(data, pixelShader->loopVars[i].offset);
        write(data, pixelShader->loopVars[i].value);
    }

    // write sampler vars
    write(data, pixelShader->samplerVarCount);
    for (uint32_t i = 0; i < pixelShader->samplerVarCount; i++) {
        writeString(data, pixelShader->samplerVars[i].name);
        write(data, pixelShader->samplerVars[i].type);
        write(data, pixelShader->samplerVars[i].location);
    }

    // write gx2rBuffer
    writeGX2RBuffer(data, &pixelShader->gx2rBuffer);

    return data;
}

template<typename Type>
Type readAt(const std::vector<uint8_t>& data, size_t& pos) {
    Type value = *reinterpret_cast<const Type*>(data.data() + pos);
    pos += sizeof(Type);
    return value;
}

const char* readString(const std::vector<uint8_t>& data, size_t& pos) {
    std::string str(reinterpret_cast<const char*>(data.data() + pos));
    pos += str.size() + 1;
    pos = (pos + 3) & ~3; // align to 4 bytes

    // Allocate memory for the string and copy the contents
    char* result = new char[str.size() + 1];
    strcpy(result, str.c_str());
    return result;
}

GX2RBuffer readGX2RBuffer(const std::vector<uint8_t>& data, size_t& pos) {
    GX2RBuffer buffer;
    buffer.flags = readAt<GX2RResourceFlags>(data, pos);
    buffer.elemSize = readAt<uint32_t>(data, pos);
    buffer.elemCount = readAt<uint32_t>(data, pos);
    size_t bufferSize = buffer.elemSize * buffer.elemCount;
    buffer.buffer = new uint8_t[bufferSize];
    memcpy(buffer.buffer, data.data() + pos, bufferSize);
    pos += bufferSize;
    return buffer;
}

GX2VertexShader* DeserializeVertexShader(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    GX2VertexShader* vertexShader = new GX2VertexShader;

    // read regs
    vertexShader->regs.sq_pgm_resources_vs = readAt<uint32_t>(data, pos);
    vertexShader->regs.vgt_primitiveid_en = readAt<uint32_t>(data, pos);
    vertexShader->regs.spi_vs_out_config = readAt<uint32_t>(data, pos);
    vertexShader->regs.num_spi_vs_out_id = readAt<uint32_t>(data, pos);
    for (uint32_t& spi_vs : vertexShader->regs.spi_vs_out_id) {
        spi_vs = readAt<uint32_t>(data, pos);
    }
    vertexShader->regs.pa_cl_vs_out_cntl = readAt<uint32_t>(data, pos);
    vertexShader->regs.sq_vtx_semantic_clear = readAt<uint32_t>(data, pos);
    vertexShader->regs.num_sq_vtx_semantic = readAt<uint32_t>(data, pos);
    for (uint32_t& sq_vtx : vertexShader->regs.sq_vtx_semantic) {
        sq_vtx = readAt<uint32_t>(data, pos);
    }
    vertexShader->regs.vgt_strmout_buffer_en = readAt<uint32_t>(data, pos);
    vertexShader->regs.vgt_vertex_reuse_block_cntl = readAt<uint32_t>(data, pos);
    vertexShader->regs.vgt_hos_reuse_depth = readAt<uint32_t>(data, pos);

    // read program
    vertexShader->size = readAt<uint32_t>(data, pos);
    vertexShader->program = aligned_alloc(256, vertexShader->size);
    for (uint32_t i = 0; i < vertexShader->size; i++) {
        static_cast<uint8_t*>(vertexShader->program)[i] = readAt<uint8_t>(data, pos);
    }
    vertexShader->mode = readAt<GX2ShaderMode>(data, pos);

    // read uniform blocks
    vertexShader->uniformBlockCount = readAt<uint32_t>(data, pos);
    vertexShader->uniformBlocks = new GX2UniformBlock[vertexShader->uniformBlockCount];
    for (uint32_t i = 0; i < vertexShader->uniformBlockCount; i++) {
        vertexShader->uniformBlocks[i].name = readString(data, pos);
        vertexShader->uniformBlocks[i].offset = readAt<uint32_t>(data, pos);
        vertexShader->uniformBlocks[i].size = readAt<uint32_t>(data, pos);
    }

    // read uniform vars
    vertexShader->uniformVarCount = readAt<uint32_t>(data, pos);
    vertexShader->uniformVars = new GX2UniformVar[vertexShader->uniformVarCount];
    for (uint32_t i = 0; i < vertexShader->uniformVarCount; i++) {
        vertexShader->uniformVars[i].name = readString(data, pos);
        vertexShader->uniformVars[i].type = readAt<GX2ShaderVarType>(data, pos);
        vertexShader->uniformVars[i].count = readAt<uint32_t>(data, pos);
        vertexShader->uniformVars[i].offset = readAt<uint32_t>(data, pos);
        vertexShader->uniformVars[i].block = readAt<int32_t>(data, pos);
    }

    // read initial values
    vertexShader->initialValueCount = readAt<uint32_t>(data, pos);
    vertexShader->initialValues = new GX2UniformInitialValue[vertexShader->initialValueCount];
    for (uint32_t i = 0; i < vertexShader->initialValueCount; i++) {
        vertexShader->initialValues[i].value[0] = readAt<float>(data, pos);
        vertexShader->initialValues[i].value[1] = readAt<float>(data, pos);
        vertexShader->initialValues[i].value[2] = readAt<float>(data, pos);
        vertexShader->initialValues[i].value[3] = readAt<float>(data, pos);
        vertexShader->initialValues[i].offset = readAt<uint32_t>(data, pos);
    }

    // read loop vars
    vertexShader->loopVarCount = readAt<uint32_t>(data, pos);
    vertexShader->loopVars = new GX2LoopVar[vertexShader->loopVarCount];
    for (uint32_t i = 0; i < vertexShader->loopVarCount; i++) {
        vertexShader->loopVars[i].offset = readAt<uint32_t>(data, pos);
        vertexShader->loopVars[i].value = readAt<uint32_t>(data, pos);
    }

    // read sampler vars
    vertexShader->samplerVarCount = readAt<uint32_t>(data, pos);
    vertexShader->samplerVars = new GX2SamplerVar[vertexShader->samplerVarCount];
    for (uint32_t i = 0; i < vertexShader->samplerVarCount; i++) {
        vertexShader->samplerVars[i].name = readString(data, pos);
        vertexShader->samplerVars[i].type = readAt<GX2SamplerVarType>(data, pos);
        vertexShader->samplerVars[i].location = readAt<uint32_t>(data, pos);
    }

    // read attribute vars
    vertexShader->attribVarCount = readAt<uint32_t>(data, pos);
    vertexShader->attribVars = new GX2AttribVar[vertexShader->attribVarCount];
    for (uint32_t i = 0; i < vertexShader->attribVarCount; i++) {
        vertexShader->attribVars[i].name = readString(data, pos);
        vertexShader->attribVars[i].type = readAt<GX2ShaderVarType>(data, pos);
        vertexShader->attribVars[i].count = readAt<uint32_t>(data, pos);
        vertexShader->attribVars[i].location = readAt<uint32_t>(data, pos);
    }

    // read ring item size
    vertexShader->ringItemsize = readAt<uint32_t>(data, pos);

    // read stream out
    vertexShader->hasStreamOut = readAt<BOOL>(data, pos);
    for (uint32_t& stride : vertexShader->streamOutStride) {
        stride = readAt<uint32_t>(data, pos);
    }

    // read gx2rBuffer
    vertexShader->gx2rBuffer = readGX2RBuffer(data, pos);

    return vertexShader;
}

GX2PixelShader* DeserializePixelShader(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    GX2PixelShader* pixelShader = new GX2PixelShader;

    // read regs
    pixelShader->regs.sq_pgm_resources_ps = readAt<uint32_t>(data, pos);
    pixelShader->regs.sq_pgm_exports_ps = readAt<uint32_t>(data, pos);
    pixelShader->regs.spi_ps_in_control_0 = readAt<uint32_t>(data, pos);
    pixelShader->regs.spi_ps_in_control_1 = readAt<uint32_t>(data, pos);
    pixelShader->regs.num_spi_ps_input_cntl = readAt<uint32_t>(data, pos);
    for (uint32_t& spi_ps : pixelShader->regs.spi_ps_input_cntls) {
        spi_ps = readAt<uint32_t>(data, pos);
    }
    pixelShader->regs.cb_shader_mask = readAt<uint32_t>(data, pos);
    pixelShader->regs.cb_shader_control = readAt<uint32_t>(data, pos);
    pixelShader->regs.db_shader_control = readAt<uint32_t>(data, pos);
    pixelShader->regs.spi_input_z = readAt<uint32_t>(data, pos);

    // read program
    pixelShader->size = readAt<uint32_t>(data, pos);
    pixelShader->program = aligned_alloc(256, pixelShader->size);
    for (uint32_t i = 0; i < pixelShader->size; i++) {
        ((uint8_t*)pixelShader->program)[i] = readAt<uint8_t>(data, pos);
    }
    pixelShader->mode = readAt<GX2ShaderMode>(data, pos);

    // read uniform blocks
    pixelShader->uniformBlockCount = readAt<uint32_t>(data, pos);
    pixelShader->uniformBlocks = new GX2UniformBlock[pixelShader->uniformBlockCount];
    for (uint32_t i = 0; i < pixelShader->uniformBlockCount; i++) {
        pixelShader->uniformBlocks[i].name = readString(data, pos);
        pixelShader->uniformBlocks[i].offset = readAt<uint32_t>(data, pos);
        pixelShader->uniformBlocks[i].size = readAt<uint32_t>(data, pos);
    }

    // read uniform vars
    pixelShader->uniformVarCount = readAt<uint32_t>(data, pos);
    pixelShader->uniformVars = new GX2UniformVar[pixelShader->uniformVarCount];
    for (uint32_t i = 0; i < pixelShader->uniformVarCount; i++) {
        pixelShader->uniformVars[i].name = readString(data, pos);
        pixelShader->uniformVars[i].type = readAt<GX2ShaderVarType>(data, pos);
        pixelShader->uniformVars[i].count = readAt<uint32_t>(data, pos);
        pixelShader->uniformVars[i].offset = readAt<uint32_t>(data, pos);
        pixelShader->uniformVars[i].block = readAt<int32_t>(data, pos);
    }

    // read initial values
    pixelShader->initialValueCount = readAt<uint32_t>(data, pos);
    pixelShader->initialValues = new GX2UniformInitialValue[pixelShader->initialValueCount];
    for (uint32_t i = 0; i < pixelShader->initialValueCount; i++) {
        pixelShader->initialValues[i].value[0] = readAt<float>(data, pos);
        pixelShader->initialValues[i].value[1] = readAt<float>(data, pos);
        pixelShader->initialValues[i].value[2] = readAt<float>(data, pos);
        pixelShader->initialValues[i].value[3] = readAt<float>(data, pos);
        pixelShader->initialValues[i].offset = readAt<uint32_t>(data, pos);
    }

    pixelShader->loopVarCount = readAt<uint32_t>(data, pos);
    pixelShader->loopVars = new GX2LoopVar[pixelShader->loopVarCount];
    for (uint32_t i = 0; i < pixelShader->loopVarCount; i++) {
        pixelShader->loopVars[i].offset = readAt<uint32_t>(data, pos);
        pixelShader->loopVars[i].value = readAt<uint32_t>(data, pos);
    }

    pixelShader->samplerVarCount = readAt<uint32_t>(data, pos);
    pixelShader->samplerVars = new GX2SamplerVar[pixelShader->samplerVarCount];
    for (uint32_t i = 0; i < pixelShader->samplerVarCount; i++) {
        pixelShader->samplerVars[i].name = readString(data, pos);
        pixelShader->samplerVars[i].type = readAt<GX2SamplerVarType>(data, pos);
        pixelShader->samplerVars[i].location = readAt<uint32_t>(data, pos);
    }

    pixelShader->gx2rBuffer = readGX2RBuffer(data, pos);

    return pixelShader;
}


void CompareVertexShader(GX2VertexShader* a, GX2VertexShader* b) {
    const auto compareValues = [](const char* name, auto a, auto b) {
        if (a != b) {
            WHBLogPrintf("Vertex shader %s differs: %d != %d", name, a, b);
        }
    };
    const auto compareStrings = [](const char* name, const char* a, const char* b) {
        // if (strcmp(a, b) != 0) {
        //     WHBLogPrintf("Vertex shader %s differs: %s != %s", name, a, b);
        // }
    };

    // compare regs
    compareValues("sq_pgm_resources_vs", a->regs.sq_pgm_resources_vs, b->regs.sq_pgm_resources_vs);
    compareValues("vgt_primitiveid_en", a->regs.vgt_primitiveid_en, b->regs.vgt_primitiveid_en);
    compareValues("spi_vs_out_config", a->regs.spi_vs_out_config, b->regs.spi_vs_out_config);
    compareValues("num_spi_vs_out_id", a->regs.num_spi_vs_out_id, b->regs.num_spi_vs_out_id);
    for (uint32_t i = 0; i < 4; i++) {
        compareValues("spi_vs_out_id", a->regs.spi_vs_out_id[i], b->regs.spi_vs_out_id[i]);
    }
    compareValues("pa_cl_vs_out_cntl", a->regs.pa_cl_vs_out_cntl, b->regs.pa_cl_vs_out_cntl);
    compareValues("sq_vtx_semantic_clear", a->regs.sq_vtx_semantic_clear, b->regs.sq_vtx_semantic_clear);
    compareValues("num_sq_vtx_semantic", a->regs.num_sq_vtx_semantic, b->regs.num_sq_vtx_semantic);
    for (uint32_t i = 0; i < 16; i++) {
        compareValues("sq_vtx_semantic", a->regs.sq_vtx_semantic[i], b->regs.sq_vtx_semantic[i]);
    }
    compareValues("vgt_strmout_buffer_en", a->regs.vgt_strmout_buffer_en, b->regs.vgt_strmout_buffer_en);
    compareValues("vgt_vertex_reuse_block_cntl", a->regs.vgt_vertex_reuse_block_cntl, b->regs.vgt_vertex_reuse_block_cntl);
    compareValues("vgt_hos_reuse_depth", a->regs.vgt_hos_reuse_depth, b->regs.vgt_hos_reuse_depth);

    // compare program
    // if (a->size != b->size) {
    //     WHBLogPrintf("Vertex shader size differs: %d != %d", a->size, b->size);
    // }
    // for (uint32_t i = 0; i < a->size; i++) {
    //     if (((uint8_t*)a->program)[i] != ((uint8_t*)b->program)[i]) {
    //         WHBLogPrintf("Vertex shader program differs at byte %d: %d != %d", i, ((uint8_t*)a->program)[i], ((uint8_t*)b->program)[i]);
    //     }
    // }
    compareValues("mode", a->mode, b->mode);

    // compare uniform blocks
    compareValues("uniformBlockCount", a->uniformBlockCount, b->uniformBlockCount);
    for (uint32_t i = 0; i < a->uniformBlockCount && i < b->uniformBlockCount; i++) {
        // compareStrings("uniformBlocks.name", a->uniformBlocks[i].name, b->uniformBlocks[i].name);
        compareValues("uniformBlocks.offset", a->uniformBlocks[i].offset, b->uniformBlocks[i].offset);
        compareValues("uniformBlocks.size", a->uniformBlocks[i].size, b->uniformBlocks[i].size);
    }

    // compare uniform vars
    compareValues("uniformVarCount", a->uniformVarCount, b->uniformVarCount);
    for (uint32_t i = 0; i < a->uniformVarCount && i < b->uniformVarCount; i++) {
        // compareStrings("uniformVars.name", a->uniformVars[i].name, b->uniformVars[i].name);
        compareValues("uniformVars.type", a->uniformVars[i].type, b->uniformVars[i].type);
        compareValues("uniformVars.count", a->uniformVars[i].count, b->uniformVars[i].count);
        compareValues("uniformVars.offset", a->uniformVars[i].offset, b->uniformVars[i].offset);
        compareValues("uniformVars.block", a->uniformVars[i].block, b->uniformVars[i].block);
    }

    // compare initial values
    compareValues("initialValueCount", a->initialValueCount, b->initialValueCount);
    for (uint32_t i = 0; i < a->initialValueCount && i < b->initialValueCount; i++) {
        for (uint32_t j = 0; j < 4; j++) {
            compareValues("initialValues.value", a->initialValues[i].value[j], b->initialValues[i].value[j]);
        }
        compareValues("initialValues.offset", a->initialValues[i].offset, b->initialValues[i].offset);
    }

    // compare loop vars
    compareValues("loopVarCount", a->loopVarCount, b->loopVarCount);
    for (uint32_t i = 0; i < a->loopVarCount && i < b->loopVarCount; i++) {
        compareValues("loopVars.offset", a->loopVars[i].offset, b->loopVars[i].offset);
        compareValues("loopVars.value", a->loopVars[i].value, b->loopVars[i].value);
    }

    // compare sampler vars
    compareValues("samplerVarCount", a->samplerVarCount, b->samplerVarCount);
    for (uint32_t i = 0; i < a->samplerVarCount && i < b->samplerVarCount; i++) {
        // compareStrings("samplerVars.name", a->samplerVars[i].name, b->samplerVars[i].name);
        compareValues("samplerVars.type", a->samplerVars[i].type, b->samplerVars[i].type);
        compareValues("samplerVars.location", a->samplerVars[i].location, b->samplerVars[i].location);
    }

    // compare attribute vars
    compareValues("attribVarCount", a->attribVarCount, b->attribVarCount);
    for (uint32_t i = 0; i < a->attribVarCount && i < b->attribVarCount; i++) {
        compareStrings("attribVars.name", a->attribVars[i].name, b->attribVars[i].name);
        compareValues("attribVars.type", a->attribVars[i].type, b->attribVars[i].type);
        compareValues("attribVars.count", a->attribVars[i].count, b->attribVars[i].count);
        compareValues("attribVars.location", a->attribVars[i].location, b->attribVars[i].location);
    }

    // compare ring item size
    compareValues("ringItemsize", a->ringItemsize, b->ringItemsize);

    // compare stream out
    compareValues("hasStreamOut", a->hasStreamOut, b->hasStreamOut);
    for (uint32_t i = 0; i < 4; i++) {
        compareValues("streamOutStride", a->streamOutStride[i], b->streamOutStride[i]);
    }

    // compare gx2rBuffer
    compareValues("gx2rBuffer.flags", a->gx2rBuffer.flags, b->gx2rBuffer.flags);
}

void ComparePixelShader(GX2PixelShader* a, GX2PixelShader* b) {
    const auto compareValues = [](const char* name, auto a, auto b) {
        if (a != b) {
            WHBLogPrintf("Pixel shader %s differs: %d != %d", name, a, b);
        }
    };
    const auto compareStrings = [](const char* name, const char* a, const char* b) {
        // if (strcmp(a, b) != 0) {
        //     WHBLogPrintf("Pixel shader %s differs: %s != %s", name, a, b);
        // }
    };

    // compare regs
    compareValues("sq_pgm_resources_ps", a->regs.sq_pgm_resources_ps, b->regs.sq_pgm_resources_ps);
    compareValues("sq_pgm_exports_ps", a->regs.sq_pgm_exports_ps, b->regs.sq_pgm_exports_ps);
    compareValues("spi_ps_in_control_0", a->regs.spi_ps_in_control_0, b->regs.spi_ps_in_control_0);
    compareValues("spi_ps_in_control_1", a->regs.spi_ps_in_control_1, b->regs.spi_ps_in_control_1);
    compareValues("num_spi_ps_input_cntl", a->regs.num_spi_ps_input_cntl, b->regs.num_spi_ps_input_cntl);
    for (uint32_t i = 0; i < 16; i++) {
        compareValues("spi_ps_input_cntls", a->regs.spi_ps_input_cntls[i], b->regs.spi_ps_input_cntls[i]);
    }
    compareValues("cb_shader_mask", a->regs.cb_shader_mask, b->regs.cb_shader_mask);
    compareValues("cb_shader_control", a->regs.cb_shader_control, b->regs.cb_shader_control);
    compareValues("db_shader_control", a->regs.db_shader_control, b->regs.db_shader_control);
    compareValues("spi_input_z", a->regs.spi_input_z, b->regs.spi_input_z);

    // compare program

    // compare uniform blocks
    compareValues("uniformBlockCount", a->uniformBlockCount, b->uniformBlockCount);
    for (uint32_t i = 0; i < a->uniformBlockCount && i < b->uniformBlockCount; i++) {
        // compareStrings("uniformBlocks.name", a->uniformBlocks[i].name, b->uniformBlocks[i].name);
        compareValues("uniformBlocks.offset", a->uniformBlocks[i].offset, b->uniformBlocks[i].offset);
        compareValues("uniformBlocks.size", a->uniformBlocks[i].size, b->uniformBlocks[i].size);
    }

    // compare uniform vars
    compareValues("uniformVarCount", a->uniformVarCount, b->uniformVarCount);
    for (uint32_t i = 0; i < a->uniformVarCount && i < b->uniformVarCount; i++) {
        compareStrings("uniformVars.name", a->uniformVars[i].name, b->uniformVars[i].name);
        compareValues("uniformVars.type", a->uniformVars[i].type, b->uniformVars[i].type);
        compareValues("uniformVars.count", a->uniformVars[i].count, b->uniformVars[i].count);
        compareValues("uniformVars.offset", a->uniformVars[i].offset, b->uniformVars[i].offset);
        compareValues("uniformVars.block", a->uniformVars[i].block, b->uniformVars[i].block);
    }

    // compare initial values
    compareValues("initialValueCount", a->initialValueCount, b->initialValueCount);
    for (uint32_t i = 0; i < a->initialValueCount && i < b->initialValueCount; i++) {
        for (uint32_t j = 0; j < 4; j++) {
            compareValues("initialValues.value", a->initialValues[i].value[j], b->initialValues[i].value[j]);
        }
        compareValues("initialValues.offset", a->initialValues[i].offset, b->initialValues[i].offset);
    }

    // compare loop vars
    compareValues("loopVarCount", a->loopVarCount, b->loopVarCount);
    for (uint32_t i = 0; i < a->loopVarCount && i < b->loopVarCount; i++) {
        compareValues("loopVars.offset", a->loopVars[i].offset, b->loopVars[i].offset);
        compareValues("loopVars.value", a->loopVars[i].value, b->loopVars[i].value);
    }

    // compare sampler vars
    compareValues("samplerVarCount", a->samplerVarCount, b->samplerVarCount);
    for (uint32_t i = 0; i < a->samplerVarCount && i < b->samplerVarCount; i++) {
        // compareStrings("samplerVars.name", a->samplerVars[i].name, b->samplerVars[i].name);
        compareValues("samplerVars.type", a->samplerVars[i].type, b->samplerVars[i].type);
        compareValues("samplerVars.location", a->samplerVars[i].location, b->samplerVars[i].location);
    }

    // compare gx2rBuffer
    compareValues("gx2rBuffer.flags", a->gx2rBuffer.flags, b->gx2rBuffer.flags);
}