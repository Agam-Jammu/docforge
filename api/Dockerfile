FROM mcr.microsoft.com/dotnet/sdk:10.0 AS build
WORKDIR /src

# Copy project file and restore
COPY DocForge.Api/DocForge.Api.csproj .
RUN dotnet restore

# Copy source and build
COPY DocForge.Api/ .
RUN dotnet publish -c Release -o /app

# Runtime image
FROM mcr.microsoft.com/dotnet/aspnet:10.0
WORKDIR /app
COPY --from=build /app ./

EXPOSE 8080
ENV ASPNETCORE_URLS=http://+:8080
ENTRYPOINT ["./DocForge.Api"]