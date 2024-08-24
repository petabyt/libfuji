set -e

models=("fuji_x_h1" "fuji_x_a2" "fuji_x_t20" "fuji_x_t2" "fuji_x_s10" "fuji_x_f10" "fuji_x30")

for model in "${models[@]}"; do
	vcam $model --ip 0.0.0.0 &
	pid=$!
	desktop/fudge.out -tw
	kill $pid
done
