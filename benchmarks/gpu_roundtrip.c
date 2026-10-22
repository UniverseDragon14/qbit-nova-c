#define _POSIX_C_SOURCE 200809L
#include <vulkan/vulkan.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MUL 1664525u
#define ADD 1013904223u
#define VKOK(x) do { VkResult r_=(x); if(r_!=VK_SUCCESS){fprintf(stderr,"Vulkan failure at %s: %d\n",#x,(int)r_);exit(2);} } while(0)
typedef struct { VkBuffer b; VkDeviceMemory m; void *map; } Buf;
typedef struct {
 VkInstance inst; VkPhysicalDevice phys; VkDevice dev; VkQueue q; uint32_t qf;
 VkCommandPool pool; VkCommandBuffer cmd; VkFence fence; VkDescriptorSetLayout dsl;
 VkPipelineLayout pl; VkShaderModule sm; VkPipeline pipe;
 VkPhysicalDeviceMemoryProperties mem; int fence_used;
} Gpu;
static uint64_t ticks(void) {
 struct timespec t;
 if(clock_gettime(CLOCK_MONOTONIC_RAW,&t)){perror("clock_gettime");exit(2);}
 return(uint64_t)t.tv_sec*1000000000u+(uint64_t)t.tv_nsec;
}
static uint32_t envn(const char*n,uint32_t d,uint32_t lo,uint32_t hi) {
 const char*s=getenv(n);char*e=NULL;unsigned long v;
 if(!s||!*s)return d;
 errno=0;
 v=strtoul(s,&e,10);
 if(errno||e==s||*e||v<lo||v>hi){fprintf(stderr,"%s must be %u..%u\n",n,lo,hi);exit(2);}
 return(uint32_t)v;
}
static uint32_t memtype(Gpu*g,uint32_t bits,VkMemoryPropertyFlags flags) {
 for(uint32_t i=0;i<g->mem.memoryTypeCount;i++)
  if((bits&(1u<<i))&&(g->mem.memoryTypes[i].propertyFlags&flags)==flags)return i;
 fprintf(stderr,"No Vulkan memory type with flags 0x%x\n",(unsigned)flags);exit(3);
}
static Buf buffer(Gpu*g,VkDeviceSize size,VkBufferUsageFlags usage,VkMemoryPropertyFlags flags,int map) {
 Buf b={0};
 VkBufferCreateInfo ci={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=size,.usage=usage,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
 VKOK(vkCreateBuffer(g->dev,&ci,NULL,&b.b));
 VkMemoryRequirements req;vkGetBufferMemoryRequirements(g->dev,b.b,&req);
 VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=memtype(g,req.memoryTypeBits,flags)};
 VKOK(vkAllocateMemory(g->dev,&ai,NULL,&b.m));VKOK(vkBindBufferMemory(g->dev,b.b,b.m,0));
 if(map)VKOK(vkMapMemory(g->dev,b.m,0,VK_WHOLE_SIZE,0,&b.map));
 return b;
}
static void dropbuf(Gpu*g,Buf*b) {
 if(b->map)vkUnmapMemory(g->dev,b->m);
 if(b->b)vkDestroyBuffer(g->dev,b->b,NULL);
 if(b->m)vkFreeMemory(g->dev,b->m,NULL);
 memset(b,0,sizeof(*b));
}
static uint32_t*loadspv(const char*path,size_t*n) {
 FILE*f=fopen(path,"rb");if(!f){perror(path);exit(2);}
 if(fseek(f,0,SEEK_END)){perror("fseek");exit(2);}long z=ftell(f);
 if(z<=0||(z%4)){fprintf(stderr,"Invalid SPIR-V file\n");exit(2);}
 rewind(f);uint32_t*p=malloc((size_t)z);if(!p){perror("malloc");exit(2);}
 if(fread(p,1,(size_t)z,f)!=(size_t)z){fprintf(stderr,"SPIR-V short read\n");exit(2);}
 fclose(f);*n=(size_t)z;return p;
}
static void initgpu(Gpu*g,const char*spv) {
 VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.pApplicationName="QBIT NOVA roundtrip benchmark",.apiVersion=VK_API_VERSION_1_1};
 VkInstanceCreateInfo ici={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};VKOK(vkCreateInstance(&ici,NULL,&g->inst));
 uint32_t n=0;VKOK(vkEnumeratePhysicalDevices(g->inst,&n,NULL));
 VkPhysicalDevice*ds=calloc(n,sizeof(*ds));if(!ds||!n){fprintf(stderr,"No Vulkan devices\n");exit(3);}
 VKOK(vkEnumeratePhysicalDevices(g->inst,&n,ds));int pick=-1;
 for(uint32_t i=0;i<n;i++){VkPhysicalDeviceProperties p;vkGetPhysicalDeviceProperties(ds[i],&p);if(p.deviceType==VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU&&strstr(p.deviceName,"V3D")){pick=(int)i;break;}}
 if(pick<0){fprintf(stderr,"No integrated V3D GPU; refusing CPU/lavapipe fallback\n");free(ds);exit(3);}
 g->phys=ds[pick];VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(g->phys,&props);printf("# GPU=%s\n",props.deviceName);vkGetPhysicalDeviceMemoryProperties(g->phys,&g->mem);free(ds);
 uint32_t qn=0;vkGetPhysicalDeviceQueueFamilyProperties(g->phys,&qn,NULL);VkQueueFamilyProperties*qs=calloc(qn,sizeof(*qs));if(!qs){perror("calloc");exit(2);}
 vkGetPhysicalDeviceQueueFamilyProperties(g->phys,&qn,qs);int qi=-1;for(uint32_t i=0;i<qn;i++)if(qs[i].queueCount&&(qs[i].queueFlags&VK_QUEUE_COMPUTE_BIT)){qi=(int)i;break;}free(qs);
 if(qi<0){fprintf(stderr,"V3D compute queue unavailable\n");exit(3);}g->qf=(uint32_t)qi;
 float priority=1;VkDeviceQueueCreateInfo qci={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=g->qf,.queueCount=1,.pQueuePriorities=&priority};
 VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci};VKOK(vkCreateDevice(g->phys,&dci,NULL,&g->dev));vkGetDeviceQueue(g->dev,g->qf,0,&g->q);
 VkDescriptorSetLayoutBinding bs[2]={
 {.binding=0,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT},
 {.binding=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT}};
 VkDescriptorSetLayoutCreateInfo di={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=2,.pBindings=bs};VKOK(vkCreateDescriptorSetLayout(g->dev,&di,NULL,&g->dsl));
 VkPushConstantRange range={.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT,.offset=0,.size=sizeof(uint32_t)};
 VkPipelineLayoutCreateInfo pli={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=1,.pSetLayouts=&g->dsl,.pushConstantRangeCount=1,.pPushConstantRanges=&range};VKOK(vkCreatePipelineLayout(g->dev,&pli,NULL,&g->pl));
 size_t sz;uint32_t*code=loadspv(spv,&sz);VkShaderModuleCreateInfo smi={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=sz,.pCode=code};VKOK(vkCreateShaderModule(g->dev,&smi,NULL,&g->sm));free(code);
 VkPipelineShaderStageCreateInfo stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=g->sm,.pName="main"};
 VkComputePipelineCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,.stage=stage,.layout=g->pl};VKOK(vkCreateComputePipelines(g->dev,VK_NULL_HANDLE,1,&pci,NULL,&g->pipe));
 VkCommandPoolCreateInfo cpi={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,.queueFamilyIndex=g->qf};VKOK(vkCreateCommandPool(g->dev,&cpi,NULL,&g->pool));
 VkCommandBufferAllocateInfo cai={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=g->pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};VKOK(vkAllocateCommandBuffers(g->dev,&cai,&g->cmd));
 VkFenceCreateInfo fi={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VKOK(vkCreateFence(g->dev,&fi,NULL,&g->fence));
}
static void cleanup(Gpu*g) {
 if(g->dev){vkDeviceWaitIdle(g->dev);if(g->fence)vkDestroyFence(g->dev,g->fence,NULL);if(g->pool)vkDestroyCommandPool(g->dev,g->pool,NULL);if(g->pipe)vkDestroyPipeline(g->dev,g->pipe,NULL);if(g->sm)vkDestroyShaderModule(g->dev,g->sm,NULL);if(g->pl)vkDestroyPipelineLayout(g->dev,g->pl,NULL);if(g->dsl)vkDestroyDescriptorSetLayout(g->dev,g->dsl,NULL);vkDestroyDevice(g->dev,NULL);}
 if(g->inst)vkDestroyInstance(g->inst,NULL);
}
static void gpu_once(Gpu*g,Buf*up,Buf*in,Buf*out,Buf*down,VkDescriptorSet ds,uint32_t count,VkDeviceSize bytes) {
 VkCommandBufferBeginInfo bi={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};VKOK(vkResetCommandBuffer(g->cmd,0));VKOK(vkBeginCommandBuffer(g->cmd,&bi));
 VkBufferMemoryBarrier reuse[3]={
 {.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_READ_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=in->b,.offset=0,.size=bytes},
 {.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT,.dstAccessMask=VK_ACCESS_SHADER_WRITE_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=out->b,.offset=0,.size=bytes},
 {.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_HOST_READ_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=down->b,.offset=0,.size=bytes}};
 vkCmdPipelineBarrier(g->cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,NULL,3,reuse,0,NULL);
 VkBufferCopy cp={.size=bytes};vkCmdCopyBuffer(g->cmd,up->b,in->b,1,&cp);
 VkBufferMemoryBarrier b1={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_SHADER_READ_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=in->b,.offset=0,.size=bytes};
 vkCmdPipelineBarrier(g->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,NULL,1,&b1,0,NULL);
 vkCmdBindPipeline(g->cmd,VK_PIPELINE_BIND_POINT_COMPUTE,g->pipe);vkCmdBindDescriptorSets(g->cmd,VK_PIPELINE_BIND_POINT_COMPUTE,g->pl,0,1,&ds,0,NULL);vkCmdPushConstants(g->cmd,g->pl,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(count),&count);vkCmdDispatch(g->cmd,(count+255u)/256u,1,1);
 VkBufferMemoryBarrier b2={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=out->b,.offset=0,.size=bytes};
 vkCmdPipelineBarrier(g->cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,1,&b2,0,NULL);vkCmdCopyBuffer(g->cmd,out->b,down->b,1,&cp);
 VkBufferMemoryBarrier b3={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=down->b,.offset=0,.size=bytes};
 vkCmdPipelineBarrier(g->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&b3,0,NULL);VKOK(vkEndCommandBuffer(g->cmd));
 if(g->fence_used)VKOK(vkResetFences(g->dev,1,&g->fence));
 VkSubmitInfo si={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&g->cmd};VKOK(vkQueueSubmit(g->q,1,&si,g->fence));VKOK(vkWaitForFences(g->dev,1,&g->fence,VK_TRUE,UINT64_MAX));g->fence_used=1;
}
static void transform(const uint32_t*a,uint32_t*b,size_t n){for(size_t i=0;i<n;i++)b[i]=a[i]*MUL+ADD;}
static int cmp(const void*a,const void*b){uint64_t x=*(const uint64_t*)a,y=*(const uint64_t*)b;return(x>y)-(x<y);}
static double median(uint64_t*a,uint32_t n){qsort(a,n,sizeof(*a),cmp);return n&1?(double)a[n/2]/1e3:(double)(a[n/2-1]+a[n/2])/2e3;}
static double percentile(const uint64_t*a,uint32_t n){uint32_t i=(uint32_t)((double)n*0.95);if(i>=n)i=n-1;return(double)a[i]/1e3;}
static int run_case(Gpu*g,size_t bytes,uint32_t iters,uint32_t warm){
 size_t n=bytes/sizeof(uint32_t);
 Buf up=buffer(g,bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,1);
 Buf in=buffer(g,bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,0);
 Buf out=buffer(g,bytes,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,0);
 Buf down=buffer(g,bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,1);
 VkDescriptorPoolSize ps={.type=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=2};VkDescriptorPoolCreateInfo pci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,.maxSets=1,.poolSizeCount=1,.pPoolSizes=&ps};VkDescriptorPool dp;VKOK(vkCreateDescriptorPool(g->dev,&pci,NULL,&dp));
 VkDescriptorSetAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,.descriptorPool=dp,.descriptorSetCount=1,.pSetLayouts=&g->dsl};VkDescriptorSet ds;VKOK(vkAllocateDescriptorSets(g->dev,&ai,&ds));
 VkDescriptorBufferInfo ib={.buffer=in.b,.offset=0,.range=bytes},ob={.buffer=out.b,.offset=0,.range=bytes};
 VkWriteDescriptorSet wr[2]={{.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=ds,.dstBinding=0,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&ib},{.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=ds,.dstBinding=1,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&ob}};vkUpdateDescriptorSets(g->dev,2,wr,0,NULL);
 uint32_t*a=malloc(bytes),*c=malloc(bytes),*d=malloc(bytes);uint64_t*ct=calloc(iters,sizeof(*ct)),*gt=calloc(iters,sizeof(*gt));
 if(!a||!c||!d||!ct||!gt){perror("allocate benchmark");return 0;}
 for(size_t i=0;i<n;i++)a[i]=(uint32_t)(i*2654435761u+0x1234567u);
 for(uint32_t j=0;j<warm;j++){transform(a,c,n);memcpy(up.map,a,bytes);gpu_once(g,&up,&in,&out,&down,ds,(uint32_t)n,bytes);if(memcmp(c,down.map,bytes)){uint32_t*v=(uint32_t*)down.map;for(size_t k=0;k<n;k++)if(c[k]!=v[k]){fprintf(stderr,"Warm-up GPU mismatch bytes=%zu index=%zu expected=0x%08x got=0x%08x\n",bytes,k,c[k],v[k]);break;}return 0;}}
 for(uint32_t j=0;j<iters;j++){uint64_t t=ticks();transform(a,c,n);ct[j]=ticks()-t;t=ticks();memcpy(up.map,a,bytes);gpu_once(g,&up,&in,&out,&down,ds,(uint32_t)n,bytes);memcpy(d,down.map,bytes);gt[j]=ticks()-t;if(memcmp(c,d,bytes)){fprintf(stderr,"GPU/CPU mismatch size=%zu iteration=%u\n",bytes,j);return 0;}}
 double cm=median(ct,iters),gm=median(gt,iters),cp=percentile(ct,iters),gp=percentile(gt,iters);
 printf("%zu,%zu,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%s\n",bytes,bytes*2,iters,cm,cp,gm,gp,gm>0?cm/gm:0,gm>0?((double)bytes*2/1e6)/(gm/1e6):0,cm>gm?"GPU_MEDIAN_FASTER":"CPU_MEDIAN_FASTER");
 free(a);free(c);free(d);free(ct);free(gt);vkDestroyDescriptorPool(g->dev,dp,NULL);dropbuf(g,&up);dropbuf(g,&in);dropbuf(g,&out);dropbuf(g,&down);return 1;
}
int main(int argc,char**argv){
 if(argc!=2){fprintf(stderr,"Usage: %s transform.spv\n",argv[0]);return 2;}
 uint32_t it=envn("QBIT_GPU_BENCH_ITERS",100,10,100000),warm=envn("QBIT_GPU_BENCH_WARMUP",10,0,10000);Gpu g={0};initgpu(&g,argv[1]);
 puts("bytes,roundtrip_bytes,iterations,cpu_median_us,cpu_p95_us,gpu_median_us,gpu_p95_us,cpu_over_gpu_speedup,roundtrip_MB_s,winner");
 const size_t sizes[]={4096,65536,1048576};int ok=1;for(size_t i=0;i<3;i++)if(!run_case(&g,sizes[i],it,warm)){ok=0;break;}cleanup(&g);return ok?0:1;
}
