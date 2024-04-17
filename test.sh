rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=256 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=300000000 --read=1000000" >>partitionkv.log
./db_bench  --value_size=256 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=300000000 >>partitionkv.log


rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=1024 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=75000000 --read=1000000" >>partitionkv.log
./db_bench  --value_size=1024 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=75000000 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=4096 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=18750000 --read=1000000" >>partitionkv.log
./db_bench  --value_size=4096 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=18750000 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=16384 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=4687500 --read=1000000" >>partitionkv.log
./db_bench  --value_size=16384 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=4687500 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=65536 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=1171875 --read=1000000" >>partitionkv.log
./db_bench  --value_size=65536 --benchmarks=fillrandom,stats,readrandom,stats,readseq,stats --num=1171875 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=256 --benchmarks=fillseq,stats --num=300000000 " >>partitionkv.log
./db_bench  --value_size=256 --benchmarks=fillseq,stats --num=300000000 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=1024 --benchmarks=fillseq,stats --num=75000000" >>partitionkv.log
./db_bench  --value_size=1024 --benchmarks=fillseq,stats --num=75000000 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=4096 --benchmarks=fillseq,stats --num=18750000" >>partitionkv.log
./db_bench  --value_size=4096 --benchmarks=fillseq,stats --num=18750000 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=16384 --benchmarks=fillseq,stats --num=4687500" >>partitionkv.log
./db_bench  --value_size=16384 --benchmarks=fillseq,stats --num=4687500 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
echo "./db_bench  --value_size=65536 --benchmarks=fillseq,stats --num=1171875" >>partitionkv.log
./db_bench  --value_size=65536 --benchmarks=fillseq,stats --num=1171875 >>partitionkv.log

rm -fr /mnt/pmemdir/pm_log
rm -fr /tmp/leveldbtest-1000/
