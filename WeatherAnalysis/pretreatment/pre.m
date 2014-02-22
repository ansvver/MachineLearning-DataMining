clear;clc

%
%  批量导入数据
%
for i=5:7
file_dir='D:\study\tmp\2011.';
data_name='data';
array_name=[file_dir,num2str(i)];
data=[data_name,num2str(i)];
xlsname=[array_name '.xls'] ;%表格名称
cmd=[data '=xlsread(xlsname)'];%读数据
eval(cmd);
end

%  选取所要分析的数据对象
data_all=[data5; data6; data7]

%  处理数据项，清理数据
[i,j]=size(data_all)
for n=1:i
	if isnan(data_all(n,13))
		data_all(n,13)=0;
	end
end
data_all(isnan(data_all(:,1)),:)=[]
data_all(:,19)=[]

%  选取适当的维
data_rel0=[data_all(:, 2), data_all(:, 13), data_all(:, 14), data_all(:, 20), data_all(:, 23), data_all(:, 24), data_all(:, 25), data_all(:, 30), data_all(:, 35), data_all(:, 48)]

%  对数据集进行主成分分析
[P, SCORE, latent]=princomp(data_rel0);

%  作图分析
latent_percent=100*latent/sum(latent); %将latent总和同一为100，便于调查供献率
pareto(latent_percent);%调用matla画图
xlabel('Principal Component');
ylabel('Variance Explained (%)');
print -djpeg 1;

%  F为得分矩阵，即新的数据对象集
%  其中data_rel0为相关分析的初始数据
F=data_rel0*P;

%  决策树分析之前的预处理
[i,j]=size(data_rel0)
data_deci=zeros(i,4)
for n=1:i
	if data_rel0(n,2)==0
		data_deci(n,4)=0;
	else
		data_deci(n,4)=1;
	end
end
data_deci(:,1:3)=F(:,1:3);

%
%  对前3维进行分级处理，得出5个等级，用1-5标识
%
max_mat=max(data_deci(:,1:3));
min_mat=min(data_deci(:,1:3));
interval_mat=(max_mat-min_mat)/5;
level_mat=zeros(5,3);
for n=1:3
	for m=1:5
		level_mat(m,n)=min_mat(1,n)+m*interval_mat(1,n)
	end
end
[i,j]=size(data_deci);
data_asso=zeros(i,4);
data_asso(:,4)=data_deci(:,4);
for n=1:3
	for m=1:i
		if(data_deci(m,n)<=level_mat(1,n))
			data_asso(m,n)=1;
		elseif(data_deci(m,n)<=level_mat(2,n))
			data_asso(m,n)=2;
		elseif(data_deci(m,n)<=level_mat(3,n))
			data_asso(m,n)=3;
		elseif(data_deci(m,n)<=level_mat(4,n))
			data_asso(m,n)=4;
		elseif(data_deci(m,n)<=level_mat(5,n))
			data_asso(m,n)=5;
		end
	end
end



